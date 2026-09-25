#include "ApplicationComposition.h"

#include "InitialSyncCoordinator.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqliteQueryConfiguration.h"
#include "../infrastructure/configuration/JsonSettingsReader.h"
#include "../infrastructure/database/SqliteCoverCacheRepository.h"
#include "../infrastructure/covers/CoverFileStore.h"
#include "../infrastructure/covers/QtCoverDownloader.h"
#include <QStandardPaths>
#include <QDir>

namespace {
QString initializationFailure(const QString &stage, const QString &detail) {
    if (detail.isEmpty()) {
        return QStringLiteral("Application initialization failed while %1.").arg(stage);
    }
    return QStringLiteral("Application initialization failed while %1: %2").arg(stage, detail);
}
}

ApplicationContext createApplicationContext() {
    ApplicationContext context;
    Settings settings;
    QString settingsError;
    // Settings is packaged as a Qt resource so startup does not depend on the
    // process working directory chosen by the IDE, service manager or shell.
    JsonSettingsReader settingsReader(QStringLiteral(":/config/Settings.json"));
    if (!settingsReader.read(settings, settingsError)) {
        settings.logRetentionDays = 7;
    }
    context.logger = std::make_unique<AsyncLogger>(settings.logRetentionDays);
    context.logger->start();
    context.logger->info(LogCategory::Application, QStringLiteral("Starting application composition."));
    if (!settingsError.isEmpty()) {
        context.logger->warning(LogCategory::Configuration, settingsError);
    }
    context.database = std::make_unique<SqliteDatabase>();
    context.database->setLogger(context.logger.get());

    if (!context.database->open()) {
        context.logger->error(LogCategory::Database, context.database->lastError());
        context.initializationError = initializationFailure(
            QStringLiteral("opening the SQLite database"), context.database->lastError());
        context.database.reset();
        return context;
    }

    if (!context.database->migrate()) {
        context.logger->error(LogCategory::Migration, context.database->lastError());
        context.initializationError = initializationFailure(
            QStringLiteral("migrating the SQLite database"), context.database->lastError());
        context.database.reset();
        return context;
    }

    QString upsertQuery;
    QString readQuery;
    QString enqueuePendingQuery;
    QString readPendingQuery;
    QString updatePendingQuery;
    QString queryError;
    SqliteQueryConfiguration queryConfiguration;
    queryConfiguration.setLogger(context.logger.get());
    if (!queryConfiguration.load(queryError)) {
        context.logger->error(LogCategory::QueryConfiguration, queryError);
        context.initializationError = initializationFailure(
            QStringLiteral("loading the SQLite query configuration"), queryError);
        context.database.reset();
        return context;
    }
    SqlQueryStore upsertStore(queryConfiguration.upsertMediaPath);
    SqlQueryStore readStore(queryConfiguration.readMediaPath);
    upsertStore.setLogger(context.logger.get());
    readStore.setLogger(context.logger.get());
    if (!upsertStore.load(upsertQuery, queryError)
        || !readStore.load(readQuery, queryError)) {
        context.logger->error(LogCategory::QueryStore, queryError);
        context.initializationError = initializationFailure(
            QStringLiteral("loading the configured media queries"), queryError);
        context.database.reset();
        return context;
    }

    SqlQueryStore enqueueStore(queryConfiguration.enqueuePendingChangePath);
    SqlQueryStore readPendingStore(queryConfiguration.readPendingChangesPath);
    SqlQueryStore updatePendingStore(queryConfiguration.updatePendingChangePath);
    if (!enqueueStore.load(enqueuePendingQuery, queryError)
        || !readPendingStore.load(readPendingQuery, queryError)
        || !updatePendingStore.load(updatePendingQuery, queryError)) {
        context.initializationError = initializationFailure(
            QStringLiteral("loading the pending-change queries"), queryError);
        context.database.reset();
        return context;
    }

    auto mediaRepository = std::make_unique<SqliteMediaRepository>(
        context.database->connection(), std::move(upsertQuery), std::move(readQuery));
    mediaRepository->setLogger(context.logger.get());
    context.mediaRepository = std::move(mediaRepository);
    context.pendingChangeRepository = std::make_unique<SqlitePendingChangeRepository>(
        context.database->connection(), std::move(enqueuePendingQuery),
        std::move(readPendingQuery), std::move(updatePendingQuery));

    QString readCoverQuery, upsertCoverQuery, deleteCoverQuery, clearCoverQuery;
    SqlQueryStore readCoverStore(queryConfiguration.readCoverCachePath);
    SqlQueryStore upsertCoverStore(queryConfiguration.upsertCoverCachePath);
    SqlQueryStore deleteCoverStore(queryConfiguration.deleteCoverCachePath);
    SqlQueryStore clearCoverStore(queryConfiguration.clearCoverCachePath);
    if (!readCoverStore.load(readCoverQuery, queryError)
        || !upsertCoverStore.load(upsertCoverQuery, queryError)
        || !deleteCoverStore.load(deleteCoverQuery, queryError)
        || !clearCoverStore.load(clearCoverQuery, queryError)) {
        context.initializationError = initializationFailure(QStringLiteral("loading the cover-cache queries"), queryError);
        return context;
    }
    context.coverCacheRepository = std::make_unique<SqliteCoverCacheRepository>(
        context.database->connection(), std::move(readCoverQuery), std::move(upsertCoverQuery),
        std::move(deleteCoverQuery), std::move(clearCoverQuery));
    const QString cacheRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("covers");
    const QString temporaryRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("HaikenAnime/covers");
    context.coverFileStore = std::make_unique<CoverFileStore>(cacheRoot, settings.covers);
    context.coverDownloader = std::make_unique<QtCoverDownloader>(temporaryRoot, settings.covers);
    context.coverCoordinator = std::make_unique<CoverDownloadCoordinator>(
        *context.coverDownloader, *context.coverCacheRepository, *context.coverFileStore, settings.covers);
    context.coverCoordinator->setLogger(context.logger.get());
    context.coverQuality = settings.covers.quality;
    context.initialSync = std::make_unique<InitialSyncCoordinator>(
        context.database->databasePath(),
        QStringLiteral(":/fixtures/graphql/page-response.json"),
        queryConfiguration.upsertMediaPath, queryConfiguration.readMediaPath,
        settings.syncTimeoutMs, settings.syncIntervalMs);
    context.initialSync->setLogger(context.logger.get());
    context.logger->info(LogCategory::Application, QStringLiteral("Application composition completed."));
    return context;
}
