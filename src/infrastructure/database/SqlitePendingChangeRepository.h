#ifndef HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H
#define HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/anilist/IPendingChangeRepository.h"
#include "SqliteQueries.h"

/** Persists the AniList update outbox in SQLite. */
class SqlitePendingChangeRepository final : public IPendingChangeRepository {
public:
    /** Creates a repository with external SQL statements for the outbox operations. */
    SqlitePendingChangeRepository(QSqlDatabase database, QString enqueueQuery,
                                  QString pendingQuery, QString updateStatusQuery);
    SqlitePendingChangeRepository(QSqlDatabase database, QString enqueueQuery,
                                  QString pendingQuery, QString pendingMediaIdsQuery,
                                  QString updateStatusQuery);

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
    QString enqueueQuery_;
    QString pendingQuery_;
    QString pendingMediaIdsQuery_;
    QString updateStatusQuery_;
    QString adoptOrphanChangesQuery_ = SqliteQueries::AdoptOrphanPendingChanges;
    qint64 ownerUserId_ = 0;
    QString unsentMediaIdsQuery_ = SqliteQueries::ReadUnsentMediaIds;
};

#endif // HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H
