#ifndef HAIKENANIME_SQLITEPERSONALLISTCHANGEWRITER_H
#define HAIKENANIME_SQLITEPERSONALLISTCHANGEWRITER_H

#include <QSqlDatabase>

#include "../../application/media/IPersonalListChangeWriter.h"

class AsyncLogger;

/** Stores a personal-list edit and its AniList outbox entries in one SQLite transaction. */
class SqlitePersonalListChangeWriter final : public IPersonalListChangeWriter {
public:
    SqlitePersonalListChangeWriter(QSqlDatabase database, QString upsertMediaQuery,
                                   QString updatePersonalListQuery, QString enqueuePendingChangeQuery,
                                   QString markPersonalListRemovedQuery, QString supersedeDeletionQuery);

    [[nodiscard]] bool remove(const Media &media, const AniListPendingChange &deletion,
                              QString &error) override;
    [[nodiscard]] bool save(const Media &media, const QList<AniListPendingChange> &changes,
                            QString &error) override;

    void setLogger(AsyncLogger *logger);

private:
    QSqlDatabase database_;
    QString upsertMediaQuery_;
    QString updatePersonalListQuery_;
    QString enqueuePendingChangeQuery_;
    QString markPersonalListRemovedQuery_;
    QString supersedeDeletionQuery_;
    AsyncLogger *logger_ = nullptr;
};

#endif // HAIKENANIME_SQLITEPERSONALLISTCHANGEWRITER_H
