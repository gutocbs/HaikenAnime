#ifndef HAIKENANIME_IANILISTUPDATECLIENT_H
#define HAIKENANIME_IANILISTUPDATECLIENT_H

#include <QString>

/** Sends user-originated changes to AniList. */
class IAniListUpdateClient {
public:
    virtual ~IAniListUpdateClient() = default;

    /** Updates remote progress for one media item. */
    [[nodiscard]] virtual bool UpdateProgress(int mediaId, int progress, QString &error) = 0;

    /** Updates remote score for one media item. */
    [[nodiscard]] virtual bool UpdateScore(int mediaId, double score, QString &error) = 0;

    /** Updates remote list status for one media item. */
    [[nodiscard]] virtual bool UpdateListStatus(int mediaId, const QString &status,
                                                QString &error) = 0;

    /** Deletes one remote list entry after the caller has confirmed the operation. */
    [[nodiscard]] virtual bool DeleteListEntry(int mediaId, QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTUPDATECLIENT_H
