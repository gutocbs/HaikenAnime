#include "ApplicationSyncTaskExecutor.h"
#include "SyncTaskRequest.h"

#include "../application/anilist/AniListSyncService.h"
#include "../infrastructure/anilist/RecordedGraphQlAniListDataSource.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqliteSyncTaskStateRepository.h"
#include "../infrastructure/database/SqliteUserPreferencesRepository.h"

#include <QThread>

#include <utility>

ApplicationSyncTaskExecutor::ApplicationSyncTaskExecutor(
    QString databasePath, QString userListFixturePath, QString catalogFixturePath, QString upsertQueryPath,
    QString readQueryPath,
    QString readActiveMediaIdsQueryPath, QString markSourceRemovedQueryPath,
    QString readTaskStatesQueryPath, QString upsertTaskStateQueryPath, QString deleteTaskStateQueryPath,
    QString readUserPreferencesQueryPath, QString upsertUserPreferencesQueryPath, const int timeoutMs)
    : databasePath_(std::move(databasePath)), userListFixturePath_(std::move(userListFixturePath)),
      catalogFixturePath_(std::move(catalogFixturePath)),
      upsertQueryPath_(std::move(upsertQueryPath)), readQueryPath_(std::move(readQueryPath)),
      readActiveMediaIdsQueryPath_(std::move(readActiveMediaIdsQueryPath)),
      markSourceRemovedQueryPath_(std::move(markSourceRemovedQueryPath)),
      readTaskStatesQueryPath_(std::move(readTaskStatesQueryPath)),
      upsertTaskStateQueryPath_(std::move(upsertTaskStateQueryPath)),
      deleteTaskStateQueryPath_(std::move(deleteTaskStateQueryPath)),
      readUserPreferencesQueryPath_(std::move(readUserPreferencesQueryPath)),
      upsertUserPreferencesQueryPath_(std::move(upsertUserPreferencesQueryPath)), timeoutMs_(timeoutMs) {
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
            } else if (partition != SyncPartition::UserList && partition != SyncPartition::ActiveCatalog
                       && partition != SyncPartition::InactiveCatalog
                       && partition != SyncPartition::CompletedCatalog) {
                error = QStringLiteral("Synchronization source partition is not configured yet.");
                result.errorCategory = AniListSyncErrorCategory::InvalidData;
            } else {
                SqliteMediaRepository mediaRepository(database.connection(), upsertQueryPath_, readQueryPath_,
                                                      readActiveMediaIdsQueryPath_, markSourceRemovedQueryPath_);
                SqliteSyncTaskStateRepository taskRepository(
                    database.connection(), readTaskStatesQueryPath_, upsertTaskStateQueryPath_, deleteTaskStateQueryPath_);
                RecordedGraphQlAniListDataSource source(partition == SyncPartition::UserList
                    ? userListFixturePath_ : catalogFixturePath_);
                AniListSyncService service(source, mediaRepository, &mediaRepository, nullptr, timeoutMs_);
                const auto synchronize = [&](AniListDataSourceRequest request) {
                    return service.synchronize(
                        request, error,
                        [&taskRepository, &checkpointState](const int page, QString &checkpointError) {
                            checkpointState.confirmedPage = page;
                            return taskRepository.Upsert(checkpointState, checkpointError);
                        },
                        [job] { return job->cancelled.load(); });
                };
                bool succeeded = true;
                if (partition == SyncPartition::UserList) {
                    UserPreferences preferences;
                    bool found = false;
                    SqliteUserPreferencesRepository preferencesRepository(
                        database.connection(), readUserPreferencesQueryPath_, upsertUserPreferencesQueryPath_);
                    if (!preferencesRepository.read(preferences, found, error)) {
                        succeeded = false;
                        result.errorCategory = AniListSyncErrorCategory::Persistence;
                    } else {
                        const auto initialState = checkpointState;
                        for (const auto &listStatus : AniListStatusesForEnabledUserLists(preferences.enabledUserLists)) {
                            auto request = SyncTaskRequest::ForState(initialState);
                            request.filter.list = listStatus;
                            request.variables.insert(QStringLiteral("list"), listStatus);
                            if (!synchronize(request)) {
                                succeeded = false;
                                break;
                            }
                        }
                    }
                } else {
                    succeeded = synchronize(SyncTaskRequest::ForState(checkpointState));
                }
                result.succeeded = succeeded;
                if (succeeded) result.errorCategory = AniListSyncErrorCategory::None;
                else if (result.errorCategory == AniListSyncErrorCategory::None) result.errorCategory = service.lastErrorCategory();
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
