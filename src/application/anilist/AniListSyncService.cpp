#include "AniListSyncService.h"
#include "AniListPendingChangeProcessor.h"
#include "AniListSyncErrorClassifier.h"

#include <QElapsedTimer>
#include <QList>
#include <QSet>

namespace {
bool HasTimedOut(const QElapsedTimer &timer, const int timeoutMs) {
    return timeoutMs > 0 && timer.elapsed() >= timeoutMs;
}
}

bool AniListSyncService::synchronize(const MediaSyncFilter &filter, QString &error) {
    error.clear();
    lastErrorCategory_ = AniListSyncErrorCategory::None;
    QElapsedTimer timer;
    timer.start();
    if (filter.startingPage <= 0 || filter.perPage <= 0) {
        error = QStringLiteral("AniList synchronization pagination must use positive values.");
        lastErrorCategory_ = AniListSyncErrorCategory::InvalidData;
        return false;
    }
    MediaSyncFilter pageFilter = filter;
    QList<int> synchronizedMediaIds;
    QSet<int> seenMediaIds;

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

        if (page.currentPage != pageFilter.startingPage) {
            error = QStringLiteral("AniList data source returned page %1 while page %2 was requested.")
                        .arg(page.currentPage)
                        .arg(pageFilter.startingPage);
            lastErrorCategory_ = AniListSyncErrorCategory::InvalidData;
            return false;
        }

        if (!page.media.isEmpty() && !mediaWriter_.upsert(page.media, error)) {
            lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
            return false;
        }

        for (const auto &media : page.media) {
            if (!seenMediaIds.contains(media.Id)) {
                seenMediaIds.insert(media.Id);
                synchronizedMediaIds.append(media.Id);
            }
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
            if (!pendingProcessor_->process(mediaId, error)) {
                lastErrorCategory_ = AniListSyncErrorClassifier::Classify(error);
                return false;
            }
        }
    }
    return true;
}
