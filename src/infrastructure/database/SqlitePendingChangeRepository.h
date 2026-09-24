#ifndef HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H
#define HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/anilist/IPendingChangeRepository.h"

/** Persists the AniList update outbox in SQLite. */
class SqlitePendingChangeRepository final : public IPendingChangeRepository {
public:
    /** Creates a repository with external SQL statements for the outbox operations. */
    SqlitePendingChangeRepository(QSqlDatabase database, QString enqueueQuery,
                                  QString pendingQuery, QString updateStatusQuery);

    [[nodiscard]] bool enqueue(const AniListPendingChange &change, QString &error) override;
    [[nodiscard]] bool getPending(int mediaId, QList<AniListPendingChange> &changes,
                                  QString &error) override;
    [[nodiscard]] bool updateStatus(const AniListPendingChange &change,
                                     QString &error) override;

private:
    QSqlDatabase database_;
    QString enqueueQuery_;
    QString pendingQuery_;
    QString updateStatusQuery_;
};

#endif // HAIKENANIME_SQLITEPENDINGCHANGEREPOSITORY_H
