#include "ApplicationComposition.h"

#include "InitialSyncCoordinator.h"

#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqliteQueryConfiguration.h"

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
    context.database = std::make_unique<SqliteDatabase>();

    if (!context.database->open()) {
        context.initializationError = initializationFailure(
            QStringLiteral("opening the SQLite database"), context.database->lastError());
        context.database.reset();
        return context;
    }

    if (!context.database->migrate()) {
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
        context.initializationError = initializationFailure(
            QStringLiteral("loading the SQLite query configuration"), queryError);
        context.database.reset();
        return context;
    }
    if (!SqlQueryStore(queryConfiguration.upsertMediaPath).load(upsertQuery, queryError)
        || !SqlQueryStore(queryConfiguration.readMediaPath).load(readQuery, queryError)) {
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
    return context;
}
