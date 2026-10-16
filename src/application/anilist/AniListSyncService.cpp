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
    lastOutcome_ = {};
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
    QSet<int> observedMediaIds;
    bool isFirstPage = true;
    bool firstPageWasCompleteSnapshot = false;

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

        if (progressReporter_) progressReporter_(static_cast<int>(synchronizedMediaIds.size()));

        receivedCompleteAuthoritativeSnapshot = receivedCompleteAuthoritativeSnapshot
            || sourceResult.isCompleteAuthoritativeSnapshot;
        observedMediaIds.unite(sourceResult.observedMediaIds);
        if (isFirstPage) {
            firstPageWasCompleteSnapshot = sourceResult.isCompleteAuthoritativeSnapshot;
            isFirstPage = false;
        }

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

    // Entries hidden by local status/type filters still exist remotely, so they count as observed.
    observedMediaIds.unite(seenMediaIds);
    lastOutcome_.observedMediaIds = observedMediaIds;

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
                observedMediaIds, {}, removedCount, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }
    }
    // Only a run that finished every chunk without error, cancellation or timeout is complete.
    lastOutcome_.completeSnapshot = request.filter.startingPage == 1 && firstPageWasCompleteSnapshot;
    return true;
}
