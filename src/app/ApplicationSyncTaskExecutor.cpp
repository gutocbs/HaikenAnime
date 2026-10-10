#include "ApplicationSyncTaskExecutor.h"
#include "SyncTaskRequest.h"

#include "../application/anilist/AniListSyncService.h"
#include "../application/anilist/IAniListAuthProvider.h"
#include "../infrastructure/anilist/AniListGraphQlClient.h"
#include "../infrastructure/anilist/GraphQlAniListDataSource.h"
#include "../infrastructure/anilist/GraphQlQueryStore.h"
#include "../infrastructure/anilist/HttpFactory.h"
#include "../infrastructure/anilist/RecordedGraphQlAniListDataSource.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqliteSyncTaskStateRepository.h"
#include "../infrastructure/database/SqliteUserPreferencesRepository.h"

#include <QThread>
#include <QNetworkAccessManager>

#include <utility>

namespace {
class SnapshotAuthProvider final : public IAniListAuthProvider {
public:
    explicit SnapshotAuthProvider(AniListCredentials credentials)
        : credentials_(std::move(credentials)) {}

    [[nodiscard]] AniListCredentials credentials() const override { return credentials_; }

private:
    AniListCredentials credentials_;
};
}

ApplicationSyncTaskExecutor::ApplicationSyncTaskExecutor(
    QString databasePath, QString catalogFixturePath, QString upsertQueryPath,
    QString readQueryPath,
    QString readActiveMediaIdsQueryPath, QString markSourceRemovedQueryPath,
    QString readTaskStatesQueryPath, QString upsertTaskStateQueryPath, QString deleteTaskStateQueryPath,
    QString readUserPreferencesQueryPath, QString upsertUserPreferencesQueryPath, const int timeoutMs,
    AuthenticatedUserListConfiguration authenticatedUserList)
    : databasePath_(std::move(databasePath)), catalogFixturePath_(std::move(catalogFixturePath)),
      upsertQueryPath_(std::move(upsertQueryPath)), readQueryPath_(std::move(readQueryPath)),
      readActiveMediaIdsQueryPath_(std::move(readActiveMediaIdsQueryPath)),
      markSourceRemovedQueryPath_(std::move(markSourceRemovedQueryPath)),
      readTaskStatesQueryPath_(std::move(readTaskStatesQueryPath)),
      upsertTaskStateQueryPath_(std::move(upsertTaskStateQueryPath)),
      deleteTaskStateQueryPath_(std::move(deleteTaskStateQueryPath)),
      readUserPreferencesQueryPath_(std::move(readUserPreferencesQueryPath)),
      upsertUserPreferencesQueryPath_(std::move(upsertUserPreferencesQueryPath)), timeoutMs_(timeoutMs),
      authenticatedUserList_(std::move(authenticatedUserList)) {
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
                if (partition == SyncPartition::UserList) {
                    if (authenticatedUserList_.credentials.token.isEmpty()
                        || authenticatedUserList_.credentials.username.isEmpty()) {
                        error = QStringLiteral("AniList user-list synchronization requires authentication.");
                        result.errorCategory = AniListSyncErrorCategory::Authentication;
                    } else {
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(
                                QStringLiteral("AniList authenticated user-list synchronization started."));
                        }
                        auto networkManager = std::unique_ptr<QNetworkAccessManager>(
                            HttpFactory::createNetworkAccessManager(nullptr));
                        SnapshotAuthProvider authProvider(authenticatedUserList_.credentials);
                        AniListGraphQlClient client(*networkManager, &authProvider,
                            authenticatedUserList_.endpoint, authenticatedUserList_.httpTimeoutMs,
                            authenticatedUserList_.httpMaxRetries,
                            authenticatedUserList_.httpRetryDelayMs,
                            {.log = authenticatedUserList_.auditLogger,
                             .responseCaptureDirectory = authenticatedUserList_.responseCaptureDirectory});
                        GraphQlQueryStore queryStore(QStringLiteral(":/anilist/queries/user-anime-list.graphql"));
                        GraphQlAniListDataSource userListSource(client, queryStore, &authProvider,
                                                                  authenticatedUserList_.perChunk);
                        AniListSyncService service(userListSource, mediaRepository, &mediaRepository,
                                                    nullptr, timeoutMs_);
                        UserPreferences preferences;
                        bool found = false;
                        SqliteUserPreferencesRepository preferencesRepository(
                            database.connection(), readUserPreferencesQueryPath_, upsertUserPreferencesQueryPath_);
                        if (!preferencesRepository.read(preferences, found, error)) {
                            result.succeeded = false;
                            result.errorCategory = AniListSyncErrorCategory::Persistence;
                        } else {
                            result.succeeded = true;
                            const auto initialState = checkpointState;
                            for (const auto &listStatus : AniListStatusesForEnabledUserLists(preferences.enabledUserLists)) {
                                auto request = SyncTaskRequest::ForState(initialState);
                                request.filter.list = listStatus;
                                if (!service.synchronize(
                                        request, error,
                                        [&taskRepository, &checkpointState](const int page, QString &checkpointError) {
                                            checkpointState.confirmedPage = page;
                                            return taskRepository.Upsert(checkpointState, checkpointError);
                                        },
                                        [job] { return job->cancelled.load(); })) {
                                    result.succeeded = false;
                                    break;
                                }
                            }
                            result.errorCategory = result.succeeded ? AniListSyncErrorCategory::None
                                : service.lastErrorCategory();
                        }
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(result.succeeded
                                ? QStringLiteral("AniList authenticated user-list synchronization completed.")
                                : QStringLiteral("AniList authenticated user-list synchronization failed."));
                        }
                    }
                } else {
                    RecordedGraphQlAniListDataSource source(catalogFixturePath_);
                    AniListSyncService service(source, mediaRepository, &mediaRepository, nullptr, timeoutMs_);
                    const auto request = SyncTaskRequest::ForState(checkpointState);
                    result.succeeded = service.synchronize(
                        request, error,
                        [&taskRepository, &checkpointState](const int page, QString &checkpointError) {
                            checkpointState.confirmedPage = page;
                            return taskRepository.Upsert(checkpointState, checkpointError);
                        },
                        [job] { return job->cancelled.load(); });
                    result.errorCategory = result.succeeded ? AniListSyncErrorCategory::None
                        : service.lastErrorCategory();
                }
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
