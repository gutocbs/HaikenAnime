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

InitialSyncCoordinator::~InitialSyncCoordinator() {
    if (thread_) {
        thread_->wait();
        delete thread_;
        thread_ = nullptr;
    }
}

void InitialSyncCoordinator::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

void InitialSyncCoordinator::start() {
    if (thread_ && thread_->isRunning()) {
        return;
    }
    if (thread_) {
        delete thread_;
        thread_ = nullptr;
    }
    if (logger_) {
        logger_->info(LogCategory::Sync, QStringLiteral("Initial synchronization started."));
    }
    emit started();
    thread_ = QThread::create([this]() {
        QString error;
        SqliteDatabase database(databasePath_);
        database.setLogger(logger_);
        if (!database.open() || !database.migrate()) {
            error = database.lastError();
            if (logger_) logger_->error(LogCategory::Sync, error);
        } else {
            QString upsertQuery;
            QString readQuery;
            SqlQueryStore upsertStore(upsertQueryPath_);
            SqlQueryStore readStore(readQueryPath_);
            upsertStore.setLogger(logger_);
            readStore.setLogger(logger_);
            if (!upsertStore.load(upsertQuery, error)
                || !readStore.load(readQuery, error)) {
                if (logger_) logger_->error(LogCategory::QueryStore, error);
            } else {
                SqliteMediaRepository repository(database.connection(), std::move(upsertQuery), std::move(readQuery));
                repository.setLogger(logger_);
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
    thread_->start();
}
