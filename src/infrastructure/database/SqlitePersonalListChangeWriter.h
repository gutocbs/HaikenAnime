#ifndef HAIKENANIME_SQLITEPERSONALLISTCHANGEWRITER_H
#define HAIKENANIME_SQLITEPERSONALLISTCHANGEWRITER_H

#include <QSqlDatabase>

#include "../../application/media/IPersonalListChangeWriter.h"

/** Stores a personal-list edit and its AniList outbox entries in one SQLite transaction. */
class SqlitePersonalListChangeWriter final : public IPersonalListChangeWriter {
public:
    SqlitePersonalListChangeWriter(QSqlDatabase database, QString upsertMediaQuery,
                                   QString updatePersonalListQuery, QString enqueuePendingChangeQuery);

    [[nodiscard]] bool save(const Media &media, const QList<AniListPendingChange> &changes,
                            QString &error) override;

private:
    QSqlDatabase database_;
    QString upsertMediaQuery_;
    QString updatePersonalListQuery_;
    QString enqueuePendingChangeQuery_;
};

#endif // HAIKENANIME_SQLITEPERSONALLISTCHANGEWRITER_H
