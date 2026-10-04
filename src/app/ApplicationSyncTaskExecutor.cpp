#include "ApplicationSyncTaskExecutor.h"

#include "../application/anilist/AniListSyncService.h"
#include "../infrastructure/anilist/RecordedGraphQlAniListDataSource.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqliteSyncTaskStateRepository.h"

#include <QThread>

#include <utility>

ApplicationSyncTaskExecutor::ApplicationSyncTaskExecutor(
    QString databasePath, QString fixturePath, QString upsertQueryPath, QString readQueryPath,
    QString readActiveMediaIdsQueryPath, QString markSourceRemovedQueryPath,
    QString readTaskStatesQueryPath, QString upsertTaskStateQueryPath, QString deleteTaskStateQueryPath,
    const int timeoutMs)
    : databasePath_(std::move(databasePath)), fixturePath_(std::move(fixturePath)),
      upsertQueryPath_(std::move(upsertQueryPath)), readQueryPath_(std::move(readQueryPath)),
      readActiveMediaIdsQueryPath_(std::move(readActiveMediaIdsQueryPath)),
      markSourceRemovedQueryPath_(std::move(markSourceRemovedQueryPath)),
      readTaskStatesQueryPath_(std::move(readTaskStatesQueryPath)),
      upsertTaskStateQueryPath_(std::move(upsertTaskStateQueryPath)),
      deleteTaskStateQueryPath_(std::move(deleteTaskStateQueryPath)), timeoutMs_(timeoutMs) {
}

ApplicationSyncTaskExecutor::~ApplicationSyncTaskExecutor() {
    std::vector<QThread *> threads;
    {
        std::lock_guard lock(mutex_);
        for (const auto &[partition, job] : jobs_) {
            job->cancelled = true;
            threads.push_back(job->thread);
        }
    }
    for (auto *thread : threads) {
        if (thread) thread->wait();
    }
}

void ApplicationSyncTaskExecutor::Execute(const SyncTaskState &state, const qint64 generation,
                                          Completion completion) {
    auto job = std::make_shared<Job>();
    const auto partition = state.partition;
    job->thread = QThread::create([this, job, state, generation, partition, completion] {
        SyncTaskExecutionResult result;
        SyncTaskState checkpointState = state;
        checkpointState.generation = generation;
        QString error;
        {
            SqliteDatabase database(databasePath_);
            if (!database.open() || !database.migrate()) {
                error = database.lastError();
                result.errorCategory = AniListSyncErrorCategory::Persistence;
            } else if (partition != SyncPartition::UserList) {
                error = QStringLiteral("Synchronization source partition is not configured yet.");
                result.errorCategory = AniListSyncErrorCategory::InvalidData;
            } else {
                SqliteMediaRepository mediaRepository(database.connection(), upsertQueryPath_, readQueryPath_,
                                                      readActiveMediaIdsQueryPath_, markSourceRemovedQueryPath_);
                SqliteSyncTaskStateRepository taskRepository(
                    database.connection(), readTaskStatesQueryPath_, upsertTaskStateQueryPath_, deleteTaskStateQueryPath_);
                RecordedGraphQlAniListDataSource source(fixturePath_);
                AniListSyncService service(source, mediaRepository, &mediaRepository, nullptr, timeoutMs_);
                MediaSyncFilter filter;
                filter.partition = partition;
                const bool succeeded = service.synchronize(
                    filter, error,
                    [&taskRepository, &checkpointState](const int page, QString &checkpointError) {
                        checkpointState.confirmedPage = page;
                        return taskRepository.Upsert(checkpointState, checkpointError);
                    },
                    [job] { return job->cancelled.load(); });
                result.succeeded = succeeded;
                result.errorCategory = succeeded ? AniListSyncErrorCategory::None : service.lastErrorCategory();
                result.confirmedPage = checkpointState.confirmedPage;
            }
        }
        if (job->cancelled.load()) {
            result.succeeded = false;
            result.errorCategory = AniListSyncErrorCategory::Cancelled;
            if (error.isEmpty()) error = QStringLiteral("Synchronization task cancelled.");
        }
        result.safeErrorDetail = error;
        Finish(job, partition, completion, std::move(result));
    });
    {
        std::lock_guard lock(mutex_);
        jobs_[partition] = job;
    }
    QObject::connect(job->thread, &QThread::finished, job->thread, &QObject::deleteLater);
    job->thread->start(QThread::LowPriority);
}

void ApplicationSyncTaskExecutor::Cancel(const SyncPartition partition,
                                          CancellationAcknowledgement acknowledgement) {
    std::shared_ptr<Job> job;
    {
        std::lock_guard lock(mutex_);
        const auto iterator = jobs_.find(partition);
        if (iterator == jobs_.end()) {
            acknowledgement();
            return;
        }
        job = iterator->second;
        job->cancelled = true;
        job->cancellationAcknowledgement = std::move(acknowledgement);
    }
}

void ApplicationSyncTaskExecutor::Shutdown(ShutdownAcknowledgement acknowledgement) {
    {
        std::lock_guard lock(mutex_);
        shutdownAcknowledgement_ = std::move(acknowledgement);
        for (const auto &[partition, job] : jobs_) job->cancelled = true;
    }
    NotifyShutdownIfIdle();
}

void ApplicationSyncTaskExecutor::Finish(const std::shared_ptr<Job> &job, const SyncPartition partition,
                                          const Completion &completion, SyncTaskExecutionResult result) {
    CancellationAcknowledgement cancellationAcknowledgement;
    {
        std::lock_guard lock(mutex_);
        const auto iterator = jobs_.find(partition);
        if (iterator != jobs_.end() && iterator->second == job) jobs_.erase(iterator);
        cancellationAcknowledgement = std::move(job->cancellationAcknowledgement);
    }
    completion(result);
    if (cancellationAcknowledgement) cancellationAcknowledgement();
    NotifyShutdownIfIdle();
}

void ApplicationSyncTaskExecutor::NotifyShutdownIfIdle() {
    ShutdownAcknowledgement acknowledgement;
    {
        std::lock_guard lock(mutex_);
        if (!jobs_.empty() || !shutdownAcknowledgement_) return;
        acknowledgement = std::move(shutdownAcknowledgement_);
    }
    acknowledgement();
}
