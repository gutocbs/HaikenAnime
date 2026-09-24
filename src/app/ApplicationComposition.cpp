#include "ApplicationComposition.h"

#include "InitialSyncCoordinator.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqliteQueryConfiguration.h"
#include "../infrastructure/configuration/JsonSettingsReader.h"

#include <QCoreApplication>
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
    context.initialSync = std::make_unique<InitialSyncCoordinator>(
        context.database->databasePath(),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../tests/fixtures/media-library.json")),
        queryConfiguration.upsertMediaPath, queryConfiguration.readMediaPath,
        settings.syncTimeoutMs, settings.syncIntervalMs);
    context.initialSync->setLogger(context.logger.get());
    context.logger->info(LogCategory::Application, QStringLiteral("Application composition completed."));
    return context;
}
