#include "AniListSyncService.h"
#include "AniListPendingChangeProcessor.h"
#include "AniListSyncErrorClassifier.h"

#include <QElapsedTimer>
#include <QList>

namespace {
bool HasTimedOut(const QElapsedTimer &timer, const int timeoutMs) {
    return timeoutMs > 0 && timer.elapsed() >= timeoutMs;
}
}

bool AniListSyncService::Synchronize(const MediaSyncFilter &filter, QString &error) {
    lastErrorCategory_ = AniListSyncErrorCategory::None;
    QElapsedTimer timer;
    timer.start();
    MediaSyncFilter pageFilter = filter;
    pageFilter.startingPage = qMax(1, pageFilter.startingPage);
    pageFilter.perPage = qMax(1, pageFilter.perPage);
    QList<int> synchronizedMediaIds;

    while (true) {
        if (HasTimedOut(timer, timeoutMs_)) {
            error = QStringLiteral("AniList synchronization timed out.");
            lastErrorCategory_ = AniListSyncErrorCategory::Timeout;
            return false;
        }
        MediaPage page;
        if (!dataSource_.fetchPage(pageFilter, page, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }

        if (!page.media.isEmpty() && !mediaRepository_.Upsert(page.media, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }

        for (const auto &media : page.media) {
            synchronizedMediaIds.append(media.Id);
        }

        if (HasTimedOut(timer, timeoutMs_)) {
            error = QStringLiteral("AniList synchronization timed out.");
            lastErrorCategory_ = AniListSyncErrorCategory::Timeout;
            return false;
        }

        if (!page.hasNextPage) {
            break;
        }

        pageFilter.startingPage = page.currentPage + 1;
    }

    if (pendingProcessor_ != nullptr) {
        for (const int mediaId : synchronizedMediaIds) {
            if (HasTimedOut(timer, timeoutMs_)) {
                error = QStringLiteral("AniList synchronization timed out.");
                lastErrorCategory_ = AniListSyncErrorCategory::Timeout;
                return false;
            }
            if (!pendingProcessor_->Process(mediaId, error)) {
                lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
                return false;
            }
        }
    }
    return true;
}
