#ifndef HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H
#define HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/anilist/IPendingChangeRepository.h"

/** Persists the AniList update outbox in SQLite. */
class SqlitePendingChangeRepository final : public IPendingChangeRepository {
public:
    /** Creates a repository over an opened connection; SQL comes from SqliteQueries. */
    explicit SqlitePendingChangeRepository(QSqlDatabase database);

    [[nodiscard]] bool enqueue(const AniListPendingChange &change, QString &error) override;
    [[nodiscard]] bool getPending(int mediaId, QList<AniListPendingChange> &changes,
                                  QString &error) override;
    [[nodiscard]] bool getPendingMediaIds(QList<int> &mediaIds, QString &error) override;
    [[nodiscard]] bool getMediaIdsWithUnsentChanges(QSet<int> &mediaIds, QString &error) override;
    /**
     * Restricts getPending/getPendingMediaIds to changes owned by the AniList account.
     * Zero means only changes without an owner; changes of other accounts stay queued.
     */
    void setOwnerUserId(qint64 ownerUserId);
    /** Assigns every ownerless change to ownerUserId and reports how many were adopted. */
    [[nodiscard]] bool adoptOrphanChanges(qint64 ownerUserId, int &adoptedCount, QString &error);
    [[nodiscard]] bool updateStatus(const AniListPendingChange &change,
                                     QString &error) override;

private:
    QSqlDatabase database_;
    qint64 ownerUserId_ = 0;
};

#endif // HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H
