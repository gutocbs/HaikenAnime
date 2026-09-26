#include "InitialSyncCoordinator.h"

#include "../application/anilist/AniListSyncService.h"
#include "../infrastructure/anilist/RecordedGraphQlAniListDataSource.h"
#include "../infrastructure/database/SqlQueryStore.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/logging/AsyncLogger.h"

#include <QDir>
#include <QMetaObject>
#include <QThread>
#include <QTimer>

#include <utility>

InitialSyncCoordinator::InitialSyncCoordinator(QString databasePath, QString fixturePath,
                                               QString upsertQueryPath, QString readQueryPath,
                                               QString readActiveMediaIdsQueryPath,
                                               QString markSourceRemovedQueryPath,
                                               const int syncTimeoutMs, const int syncIntervalMs,
                                               QObject *parent)
    : QObject(parent), databasePath_(std::move(databasePath)), fixturePath_(std::move(fixturePath)),
      upsertQueryPath_(std::move(upsertQueryPath)), readQueryPath_(std::move(readQueryPath)),
      readActiveMediaIdsQueryPath_(std::move(readActiveMediaIdsQueryPath)),
      markSourceRemovedQueryPath_(std::move(markSourceRemovedQueryPath)),
      syncTimeoutMs_(syncTimeoutMs), syncIntervalMs_(syncIntervalMs),
      scheduler_(new QTimer(this)) {
    scheduler_->setSingleShot(false);
    scheduler_->setInterval(syncIntervalMs_);
    connect(scheduler_, &QTimer::timeout, this, &InitialSyncCoordinator::start);
}

InitialSyncCoordinator::InitialSyncCoordinator(SyncOperation operation,
                                               const int syncIntervalMs,
                                               QObject *parent)
    : QObject(parent), syncTimeoutMs_(0), syncIntervalMs_(syncIntervalMs),
      scheduler_(new QTimer(this)), operation_(std::move(operation)) {
    scheduler_->setSingleShot(false);
    scheduler_->setInterval(syncIntervalMs_);
    connect(scheduler_, &QTimer::timeout, this, &InitialSyncCoordinator::start);
}

InitialSyncCoordinator::~InitialSyncCoordinator() {
    shutdown();
}

void InitialSyncCoordinator::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

void InitialSyncCoordinator::configureAutomaticSynchronization(const bool enabled,
                                                               const int intervalMs) {
    automaticSynchronizationEnabled_ = enabled;
    if (intervalMs > 0) syncIntervalMs_ = intervalMs;
    scheduler_->setInterval(syncIntervalMs_);
    scheduler_->stop();
    if (enabled && !stopping_ && !executionActive_) scheduler_->start();
}

bool InitialSyncCoordinator::automaticSynchronizationEnabled() const {
    return automaticSynchronizationEnabled_;
}

int InitialSyncCoordinator::synchronizationIntervalMs() const {
    return syncIntervalMs_;
}

void InitialSyncCoordinator::start() {
    if (stopping_ || executionActive_) {
        return;
    }
    releaseFinishedThread();
    executionActive_ = true;
    if (logger_) {
        logger_->info(LogCategory::Sync, QStringLiteral("Initial synchronization started."));
    }
    emit started();
    if (stopping_) {
        executionActive_ = false;
        return;
    }
    thread_ = QThread::create([this]() {
        QString error;
        const bool succeeded = operation_ ? operation_(error) : performSynchronization(error);
        if (!succeeded && error.isEmpty()) {
            error = QStringLiteral("Media synchronization failed.");
        }
        if (logger_ && !error.isEmpty()) {
            logger_->error(LogCategory::Sync, error);
        }
        QMetaObject::invokeMethod(this, [this, error = std::move(error)]() {
            executionActive_ = false;
            error.isEmpty() ? emit completed() : emit failed(error);
            if (!stopping_ && automaticSynchronizationEnabled_
                && scheduler_ && !scheduler_->isActive()) {
                scheduler_->start();
            }
        }, Qt::QueuedConnection);
    });
    thread_->start();
}

void InitialSyncCoordinator::shutdown() {
    if (!stopping_) {
        stopping_ = true;
        scheduler_->stop();
    }
    if (thread_ && thread_->isRunning()) {
        thread_->wait();
    }
    releaseFinishedThread();
}

bool InitialSyncCoordinator::performSynchronization(QString &error) const {
    SqliteDatabase database(databasePath_);
    database.setLogger(logger_);
    if (!database.open() || !database.migrate()) {
        error = database.lastError();
        return false;
    }

    QString upsertQuery;
    QString readQuery;
    QString readActiveMediaIdsQuery;
    QString markSourceRemovedQuery;
    SqlQueryStore upsertStore(upsertQueryPath_);
    SqlQueryStore readStore(readQueryPath_);
    SqlQueryStore readActiveMediaIdsStore(readActiveMediaIdsQueryPath_);
    SqlQueryStore markSourceRemovedStore(markSourceRemovedQueryPath_);
    upsertStore.setLogger(logger_);
    readStore.setLogger(logger_);
    if (!upsertStore.load(upsertQuery, error)
        || !readStore.load(readQuery, error)
        || !readActiveMediaIdsStore.load(readActiveMediaIdsQuery, error)
        || !markSourceRemovedStore.load(markSourceRemovedQuery, error)) {
        return false;
    }

    SqliteMediaRepository repository(database.connection(), std::move(upsertQuery),
                                     std::move(readQuery), std::move(readActiveMediaIdsQuery),
                                     std::move(markSourceRemovedQuery));
    repository.setLogger(logger_);
    RecordedGraphQlAniListDataSource source(QDir::cleanPath(fixturePath_));
    AniListSyncService service(source, repository, &repository, nullptr, syncTimeoutMs_);
    MediaSyncFilter filter;
    return service.synchronize(filter, error);
}

void InitialSyncCoordinator::releaseFinishedThread() {
    if (thread_ && !thread_->isRunning()) {
        delete thread_;
        thread_ = nullptr;
    }
}
