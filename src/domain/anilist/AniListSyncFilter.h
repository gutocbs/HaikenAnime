#ifndef HAIKENANIME_ANILISTSYNCFILTER_H
#define HAIKENANIME_ANILISTSYNCFILTER_H

#include <QString>

/** Optional criteria used to restrict an AniList synchronization. */
struct AniListSyncFilter {
    QString username;
    QString type;
    QString status;
    QString list;
    int startingPage = 1;
    int perPage = 50;

    /** Returns true when no media-selection criterion was provided. */
    [[nodiscard]] bool isEmpty() const {
        return username.isEmpty() && type.isEmpty() && status.isEmpty() && list.isEmpty();
    }
};

#endif // HAIKENANIME_ANILISTSYNCFILTER_H
