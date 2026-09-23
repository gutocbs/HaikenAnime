#include "ApplicationComposition.h"

#include "InitialSyncCoordinator.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
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
    JsonSettingsReader settingsReader(QStringLiteral("Settings.json"));
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
    QString queryError;
    SqliteQueryConfiguration queryConfiguration;
    if (!queryConfiguration.load(queryError)) {
        context.logger->error(LogCategory::QueryConfiguration, queryError);
        context.initializationError = initializationFailure(
            QStringLiteral("loading the SQLite query configuration"), queryError);
        context.database.reset();
        return context;
    }
    if (!SqlQueryStore(queryConfiguration.upsertMediaPath).load(upsertQuery, queryError)
        || !SqlQueryStore(queryConfiguration.readMediaPath).load(readQuery, queryError)) {
        context.logger->error(LogCategory::QueryStore, queryError);
        context.initializationError = initializationFailure(
            QStringLiteral("loading the configured media queries"), queryError);
        context.database.reset();
        return context;
    }

    context.mediaRepository = std::make_unique<SqliteMediaRepository>(
        context.database->connection(), std::move(upsertQuery), std::move(readQuery));
    context.initialSync = std::make_unique<InitialSyncCoordinator>(
        context.database->databasePath(),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../tests/fixtures/media-library.json")),
        queryConfiguration.upsertMediaPath, queryConfiguration.readMediaPath);
    context.initialSync->setLogger(context.logger.get());
    context.logger->info(LogCategory::Application, QStringLiteral("Application composition completed."));
    return context;
}
