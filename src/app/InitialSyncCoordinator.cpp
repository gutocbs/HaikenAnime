#include "InitialSyncCoordinator.h"

#include "../application/anilist/AniListSyncService.h"
#include "../infrastructure/anilist/FileAniListDataSource.h"
#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/logging/AsyncLogger.h"

#include <QDir>
#include <QMetaObject>
#include <QThread>

#include <utility>

InitialSyncCoordinator::InitialSyncCoordinator(QString databasePath, QString fixturePath,
                                               QString upsertQueryPath, QString readQueryPath, QObject *parent)
    : QObject(parent), databasePath_(std::move(databasePath)), fixturePath_(std::move(fixturePath)),
      upsertQueryPath_(std::move(upsertQueryPath)), readQueryPath_(std::move(readQueryPath)) {
}

void InitialSyncCoordinator::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

void InitialSyncCoordinator::start() {
    if (logger_) {
        logger_->info(LogCategory::Sync, QStringLiteral("Initial synchronization started."));
    }
    emit started();
    auto *thread = QThread::create([this]() {
        QString error;
        SqliteDatabase database(databasePath_);
        if (!database.open() || !database.migrate()) {
            error = database.lastError();
            if (logger_) logger_->error(LogCategory::Sync, error);
        } else {
            QString upsertQuery;
            QString readQuery;
            if (!SqlQueryStore(upsertQueryPath_).load(upsertQuery, error)
                || !SqlQueryStore(readQueryPath_).load(readQuery, error)) {
                if (logger_) logger_->error(LogCategory::QueryStore, error);
            } else {
                SqliteMediaRepository repository(database.connection(), std::move(upsertQuery), std::move(readQuery));
                FileAniListDataSource source(QDir::cleanPath(fixturePath_));
                AniListSyncService service(source, repository);
                MediaSyncFilter filter;
                if (!service.Synchronize(filter, error) && error.isEmpty()) {
                    error = QStringLiteral("Media synchronization failed.");
                }
                if (logger_ && !error.isEmpty()) logger_->error(LogCategory::Sync, error);
            }
        }
        QMetaObject::invokeMethod(this, [this, error = std::move(error)]() {
            error.isEmpty() ? emit completed() : emit failed(error);
        }, Qt::QueuedConnection);
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}
