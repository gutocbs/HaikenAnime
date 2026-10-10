#include "AniListSyncService.h"
#include "AniListPendingChangeProcessor.h"
#include "AniListPendingChangeReconciler.h"
#include "AniListSyncErrorClassifier.h"

#include <QElapsedTimer>
#include <QList>
#include <QSet>

namespace {
bool HasTimedOut(const QElapsedTimer &timer, const int timeoutMs) {
    return timeoutMs > 0 && timer.elapsed() >= timeoutMs;
}
}

bool AniListSyncService::synchronize(const MediaSyncFilter &filter, QString &error,
                                     const CheckpointCommitter &commitCheckpoint,
                                     const CancellationProbe &isCancelled) {
    AniListDataSourceRequest request;
    request.filter = filter;
    return synchronize(request, error, commitCheckpoint, isCancelled);
}

bool AniListSyncService::synchronize(const AniListDataSourceRequest &request, QString &error,
                                     const CheckpointCommitter &commitCheckpoint,
                                     const CancellationProbe &isCancelled) {
    error.clear();
    lastErrorCategory_ = AniListSyncErrorCategory::None;
    QElapsedTimer timer;
    timer.start();
    if (request.filter.startingPage <= 0 || request.filter.perPage <= 0) {
        error = QStringLiteral("AniList synchronization pagination must use positive values.");
        lastErrorCategory_ = AniListSyncErrorCategory::InvalidData;
        return false;
    }
    AniListDataSourceRequest pageRequest = request;
    bool receivedCompleteAuthoritativeSnapshot = false;
    QList<int> synchronizedMediaIds;
    QSet<int> seenMediaIds;

    while (true) {
        if (isCancelled && isCancelled()) {
            error = QStringLiteral("AniList synchronization cancelled.");
            lastErrorCategory_ = AniListSyncErrorCategory::Cancelled;
            return false;
        }
        if (HasTimedOut(timer, timeoutMs_)) {
            error = QStringLiteral("AniList synchronization timed out.");
            lastErrorCategory_ = AniListSyncErrorCategory::Timeout;
            return false;
        }
        AniListDataSourceResult sourceResult;
        if (aniListDataSource_ != nullptr) {
            if (!aniListDataSource_->fetchPage(pageRequest, sourceResult, error)) {
                lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
                return false;
            }
            if (sourceResult.completedPartition != pageRequest.filter.partition) {
                error = QStringLiteral("AniList data source completed partition %1 while partition %2 was requested.")
                            .arg(ToString(sourceResult.completedPartition), ToString(pageRequest.filter.partition));
                lastErrorCategory_ = AniListSyncErrorCategory::InvalidData;
                return false;
            }
        } else if (dataSource_ == nullptr
                   || !dataSource_->fetchPage(pageRequest.filter, sourceResult.page, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }
        if (aniListDataSource_ == nullptr) {
            sourceResult.completedPartition = pageRequest.filter.partition;
            sourceResult.isCompleteAuthoritativeSnapshot = false;
        }
        MediaPage page = std::move(sourceResult.page);
        if (isCancelled && isCancelled()) {
            error = QStringLiteral("AniList synchronization cancelled.");
            lastErrorCategory_ = AniListSyncErrorCategory::Cancelled;
            return false;
        }

        if (page.currentPage != pageRequest.filter.startingPage) {
            error = QStringLiteral("AniList data source returned page %1 while page %2 was requested.")
                        .arg(page.currentPage)
                        .arg(pageRequest.filter.startingPage);
            lastErrorCategory_ = AniListSyncErrorCategory::InvalidData;
            return false;
        }

        if (pendingReconciler_ != nullptr && !page.media.isEmpty()
            && !pendingReconciler_->reconcile(page.media, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }

        if (!page.media.isEmpty() && !mediaWriter_.upsert(page.media, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }

        if (commitCheckpoint && !commitCheckpoint(page.currentPage, error)) {
            if (error.isEmpty()) error = QStringLiteral("Unable to persist AniList synchronization checkpoint.");
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }

        for (const auto &media : page.media) {
            if (!seenMediaIds.contains(media.Id)) {
                seenMediaIds.insert(media.Id);
                synchronizedMediaIds.append(media.Id);
            }
        }

        receivedCompleteAuthoritativeSnapshot = receivedCompleteAuthoritativeSnapshot
            || sourceResult.isCompleteAuthoritativeSnapshot;

        if (HasTimedOut(timer, timeoutMs_)) {
            error = QStringLiteral("AniList synchronization timed out.");
            lastErrorCategory_ = AniListSyncErrorCategory::Timeout;
            return false;
        }

        if (!page.hasNextPage) {
            break;
        }

        pageRequest.setPage(page.currentPage + 1);
    }

    if (pendingProcessor_ != nullptr) {
        for (const int mediaId : synchronizedMediaIds) {
            if (isCancelled && isCancelled()) {
                error = QStringLiteral("AniList synchronization cancelled.");
                lastErrorCategory_ = AniListSyncErrorCategory::Cancelled;
                return false;
            }
            if (HasTimedOut(timer, timeoutMs_)) {
                error = QStringLiteral("AniList synchronization timed out.");
                lastErrorCategory_ = AniListSyncErrorCategory::Timeout;
                return false;
            }
            if (!pendingProcessor_->process(mediaId, error)) {
                lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
                return false;
            }
        }
    }
    if (request.filter.startingPage == 1 && receivedCompleteAuthoritativeSnapshot
        && snapshotReconciler_ != nullptr) {
        int removedCount = 0;
        if (!snapshotReconciler_->reconcileAuthoritativeSnapshot(
                seenMediaIds, removedCount, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }
    }
    return true;
}
