#include "ApplicationSyncTaskExecutor.h"
#include "SyncTaskRequest.h"

#include "../application/anilist/AniListAccountOwnership.h"
#include "../application/anilist/AniListSyncService.h"
#include "../application/anilist/AniListPendingChangeProcessor.h"
#include "../application/anilist/AniListPendingChangeDrain.h"
#include "../application/anilist/AniListPendingChangeReconciler.h"
#include "../application/anilist/AniListSnapshotReconciliation.h"
#include "../application/anilist/AniListSyncErrorClassifier.h"
#include "../application/anilist/IAniListAuthProvider.h"
#include "../infrastructure/anilist/AniListGraphQlClient.h"
#include "../infrastructure/anilist/GraphQlAniListDataSource.h"
#include "../infrastructure/anilist/GraphQlQueryStore.h"
#include "../infrastructure/anilist/HttpFactory.h"
#include "../infrastructure/anilist/AniListUpdateClient.h"
#include "../infrastructure/database/SqliteDatabase.h"
#include "../infrastructure/database/SqliteMediaRepository.h"
#include "../infrastructure/database/SqlitePendingChangeRepository.h"
#include "../infrastructure/database/SqliteSyncTaskStateRepository.h"
#include "../infrastructure/database/SqliteUserPreferencesRepository.h"

#include <QThread>
#include <QNetworkAccessManager>

#include <functional>
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

/** Scopes the outbox to the authenticated account and adopts ownerless legacy changes. */
bool ScopePendingChangesToAccount(SqlitePendingChangeRepository &pendingChanges, const qint64 ownerUserId,
                                  const std::function<void(const QString &)> &auditLogger, QString &error) {
    pendingChanges.setOwnerUserId(ownerUserId);
    if (ownerUserId <= 0) return true;
    int adoptedCount = 0;
    if (!pendingChanges.adoptOrphanChanges(ownerUserId, adoptedCount, error)) return false;
    if (adoptedCount > 0 && auditLogger) {
        auditLogger(QStringLiteral("Adopted %1 pending AniList changes without owner.").arg(adoptedCount));
    }
    return true;
}
}

ApplicationSyncTaskExecutor::ApplicationSyncTaskExecutor(
    QString databasePath, QString upsertQueryPath,
    QString readQueryPath,
    QString readActiveMediaIdsQueryPath, QString markSourceRemovedQueryPath,
    QString readTaskStatesQueryPath, QString upsertTaskStateQueryPath, QString deleteTaskStateQueryPath,
    QString readUserPreferencesQueryPath, QString upsertUserPreferencesQueryPath, const int timeoutMs,
    AuthenticatedUserListConfiguration authenticatedUserList)
    : databasePath_(std::move(databasePath)), upsertQueryPath_(std::move(upsertQueryPath)),
      readQueryPath_(std::move(readQueryPath)),
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

void ApplicationSyncTaskExecutor::SetProgressReporter(ProgressReporter reporter) {
    std::lock_guard lock(mutex_);
    progressReporter_ = std::move(reporter);
}

void ApplicationSyncTaskExecutor::Execute(const SyncTaskState &state, const qint64 generation,
                                          Completion completion) {
    auto job = std::make_shared<Job>();
    const auto partition = state.partition;
    ProgressReporter reportProgress;
    {
        std::lock_guard lock(mutex_);
        reportProgress = progressReporter_;
    }
    job->thread = QThread::create([this, job, state, generation, partition, completion, reportProgress] {
        const auto publish = [&reportProgress, partition](const QString &stageKey, const int stageIndex,
                                                          const int stageCount, const int processedItems,
                                                          const int totalItems) {
            if (!reportProgress) return;
            reportProgress({.partition = partition, .stageKey = stageKey, .stageIndex = stageIndex,
                            .stageCount = stageCount, .processedItems = processedItems,
                            .totalItems = totalItems});
        };
        SyncTaskExecutionResult result;
        SyncTaskState checkpointState = state;
        checkpointState.generation = generation;
        QString error;
        {
            SqliteDatabase database(databasePath_);
            if (!database.open()) {
                error = database.lastError();
                result.errorCategory = AniListSyncErrorCategory::Persistence;
            } else {
                const AniListCredentials credentials = authenticatedUserList_.credentialsProvider
                    ? authenticatedUserList_.credentialsProvider() : authenticatedUserList_.credentials;
                if (partition == SyncPartition::UserList) {
                    SqliteMediaRepository mediaRepository(database.connection(), upsertQueryPath_, readQueryPath_,
                                                          readActiveMediaIdsQueryPath_, markSourceRemovedQueryPath_);
                    SqliteSyncTaskStateRepository taskRepository(
                        database.connection(), readTaskStatesQueryPath_, upsertTaskStateQueryPath_, deleteTaskStateQueryPath_);
                    if (credentials.token.isEmpty() || credentials.username.isEmpty()) {
                        error = QStringLiteral("AniList user-list synchronization requires authentication.");
                        result.errorCategory = AniListSyncErrorCategory::Authentication;
                    } else {
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(
                                QStringLiteral("AniList authenticated user-list synchronization started."));
                        }
                        auto networkManager = std::unique_ptr<QNetworkAccessManager>(
                            HttpFactory::createNetworkAccessManager(nullptr));
                        SnapshotAuthProvider authProvider(credentials);
                        AniListGraphQlClient client(*networkManager, &authProvider,
                            authenticatedUserList_.endpoint, authenticatedUserList_.httpTimeoutMs,
                            authenticatedUserList_.httpMaxRetries,
                            authenticatedUserList_.httpRetryDelayMs,
                            {.log = authenticatedUserList_.auditLogger});
                        GraphQlQueryStore queryStore(QStringLiteral(":/anilist/queries/user-media-list.graphql"));
                        GraphQlAniListDataSource userListSource(client, queryStore, &authProvider,
                                                                  authenticatedUserList_.perChunk);
                        SqlitePendingChangeRepository pendingChanges(
                            database.connection(), QStringLiteral(":/sqlite/queries/enqueue-pending-change.sql"),
                            QStringLiteral(":/sqlite/queries/read-pending-changes.sql"),
                            QStringLiteral(":/sqlite/queries/read-pending-media-ids.sql"),
                            QStringLiteral(":/sqlite/queries/update-pending-change.sql"));
                        AniListPendingChangeReconciler pendingReconciler(
                            pendingChanges, authenticatedUserList_.auditLogger);
                        AniListSyncService service(userListSource, mediaRepository, nullptr,
                                                    nullptr, timeoutMs_, &pendingReconciler);
                        UserPreferences preferences;
                        bool found = false;
                        SqliteUserPreferencesRepository preferencesRepository(
                            database.connection(), readUserPreferencesQueryPath_, upsertUserPreferencesQueryPath_);
                        if (!ScopePendingChangesToAccount(pendingChanges, credentials.userId,
                                                          authenticatedUserList_.auditLogger, error)
                            || !preferencesRepository.read(preferences, found, error)) {
                            result.succeeded = false;
                            result.errorCategory = AniListSyncErrorCategory::Persistence;
                        } else {
                            result.succeeded = true;
                            const auto initialState = checkpointState;
                            const auto statuses = AniListStatusesForEnabledUserLists(preferences.enabledUserLists);
                            if (statuses.isEmpty() && authenticatedUserList_.auditLogger) {
                                authenticatedUserList_.auditLogger(
                                    QStringLiteral("AniList user-list synchronization skipped: no lists are enabled."));
                            }
                            QList<AniListSyncOutcome> outcomes;
                            auto snapshotErrorCategory = AniListSyncErrorCategory::None;
                            int stageIndex = 0;
                            for (const auto &mediaType : {QStringLiteral("ANIME"), QStringLiteral("MANGA")}) {
                                if (!result.succeeded || statuses.isEmpty()) break;
                                ++stageIndex;
                                const QString stageKey = stageIndex == 1 ? QStringLiteral("anime")
                                                                         : QStringLiteral("manga");
                                publish(stageKey, stageIndex, 3, 0, -1);
                                service.setProgressReporter([&publish, &stageKey, stageIndex](const int processed) {
                                    publish(stageKey, stageIndex, 3, processed, -1);
                                });
                                if (authenticatedUserList_.auditLogger) {
                                    authenticatedUserList_.auditLogger(
                                        QStringLiteral("AniList user-list request started: type=%1 statuses=%2.")
                                            .arg(mediaType, statuses.join(',')));
                                }
                                auto request = SyncTaskRequest::ForState(initialState);
                                request.filter.type = mediaType;
                                request.filter.acceptedListStatuses = statuses;
                                if (!service.synchronize(
                                        request, error,
                                        [&taskRepository, &checkpointState](const int page, QString &checkpointError) {
                                            checkpointState.confirmedPage = page;
                                            return taskRepository.Upsert(checkpointState, checkpointError);
                                        },
                                        [job] { return job->cancelled.load(); })) {
                                    result.succeeded = false;
                                    if (authenticatedUserList_.auditLogger) {
                                        authenticatedUserList_.auditLogger(
                                            QStringLiteral("AniList user-list request failed: type=%1.")
                                                .arg(mediaType));
                                    }
                                    break;
                                }
                                outcomes.append(service.lastOutcome());
                                if (authenticatedUserList_.auditLogger) {
                                    authenticatedUserList_.auditLogger(
                                        QStringLiteral("AniList user-list request completed: type=%1.")
                                            .arg(mediaType));
                                }
                            }
                            if (!statuses.isEmpty()) {
                                publish(QStringLiteral("reconciliation"), 3, 3, 0, -1);
                                int removedCount = 0;
                                bool reconciled = false;
                                if (!ReconcileCompleteAniListSnapshot(
                                        outcomes, 2, result.succeeded && !job->cancelled.load(),
                                        mediaRepository, pendingChanges, reconciled, removedCount, error)) {
                                    result.succeeded = false;
                                    snapshotErrorCategory = AniListSyncErrorClassifier::Classify(error);
                                } else if (reconciled && authenticatedUserList_.auditLogger) {
                                    authenticatedUserList_.auditLogger(
                                        QStringLiteral("AniList snapshot reconciliation marked %1 media as removed from source.")
                                            .arg(removedCount));
                                }
                            }
                            result.errorCategory = result.succeeded ? AniListSyncErrorCategory::None
                                : (snapshotErrorCategory != AniListSyncErrorCategory::None
                                       ? snapshotErrorCategory : service.lastErrorCategory());
                        }
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(result.succeeded
                                ? QStringLiteral("AniList authenticated user-list synchronization completed.")
                                : QStringLiteral("AniList authenticated user-list synchronization failed."));
                        }
                    }
                } else if (partition == SyncPartition::PendingChanges) {
                    if (credentials.token.isEmpty()) {
                        error = QStringLiteral("AniList pending-change synchronization requires authentication.");
                        result.errorCategory = AniListSyncErrorCategory::Authentication;
                    } else if (!AniListAccountOwnership::CanProcessPendingChanges(credentials.userId)) {
                        // Token not validated yet (no account id): nothing is sent or adopted until it is.
                        result.succeeded = true;
                        result.errorCategory = AniListSyncErrorCategory::None;
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(QStringLiteral(
                                "AniList pending-change synchronization skipped: account not validated yet."));
                        }
                    } else {
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(
                                QStringLiteral("AniList pending-change synchronization started."));
                        }
                        auto networkManager = std::unique_ptr<QNetworkAccessManager>(
                            HttpFactory::createNetworkAccessManager(nullptr));
                        SnapshotAuthProvider authProvider(credentials);
                        AniListGraphQlClient client(*networkManager, &authProvider,
                            authenticatedUserList_.endpoint, authenticatedUserList_.httpTimeoutMs,
                            authenticatedUserList_.httpMaxRetries,
                            authenticatedUserList_.httpRetryDelayMs,
                            {.log = authenticatedUserList_.auditLogger});
                        SqlitePendingChangeRepository pendingChanges(
                            database.connection(), QStringLiteral(":/sqlite/queries/enqueue-pending-change.sql"),
                            QStringLiteral(":/sqlite/queries/read-pending-changes.sql"),
                            QStringLiteral(":/sqlite/queries/read-pending-media-ids.sql"),
                            QStringLiteral(":/sqlite/queries/update-pending-change.sql"));
                        QString scopeError;
                        const bool scoped = ScopePendingChangesToAccount(
                            pendingChanges, credentials.userId, authenticatedUserList_.auditLogger, scopeError);
                        GraphQlQueryStore updateMutation(
                            QStringLiteral(":/anilist/mutations/update-media-list-entry.graphql"));
                        GraphQlQueryStore entryIdQuery(
                            QStringLiteral(":/anilist/queries/media-list-entry-id.graphql"));
                        GraphQlQueryStore deleteMutation(
                            QStringLiteral(":/anilist/mutations/delete-list-entry.graphql"));
                        AniListUpdateClient updateClient(client, updateMutation, entryIdQuery, deleteMutation);
                        AniListPendingChangeProcessor pendingProcessor(
                            pendingChanges, updateClient, credentials.userId);
                        AniListPendingChangeDrain drain(pendingChanges, pendingProcessor);
                        drain.setProgressReporter([&publish](const int processed, const int total) {
                            publish(QStringLiteral("pending_changes"), 1, 1, processed, total);
                        });
                        result.succeeded = scoped && drain.processAll(error,
                            [job] { return job->cancelled.load(); });
                        if (!scoped) error = scopeError;
                        result.errorCategory = result.succeeded ? AniListSyncErrorCategory::None
                            : (scoped ? AniListSyncErrorClassifier::Classify(error)
                                      : AniListSyncErrorCategory::Persistence);
                        if (authenticatedUserList_.auditLogger) {
                            authenticatedUserList_.auditLogger(result.succeeded
                                ? QStringLiteral("AniList pending-change synchronization completed.")
                                : QStringLiteral("AniList pending-change synchronization failed."));
                        }
                    }
                } else {
                    error = QStringLiteral("Synchronization source partition is not configured yet.");
                    result.errorCategory = AniListSyncErrorCategory::InvalidData;
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
