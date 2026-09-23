#ifndef HAIKENANIME_SQLITEMEDIAREPOSITORY_H
#define HAIKENANIME_SQLITEMEDIAREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/anilist/IMediaRepository.h"

/** Persists AniList media in the application's SQLite database. */
class SqliteMediaRepository final : public IMediaRepository {
public:
    /** Creates a repository using an opened database connection and external upsert SQL. */
    SqliteMediaRepository(QSqlDatabase database, QString upsertQuery);

    /** Upserts external media in one transaction while preserving local user fields. */
    [[nodiscard]] bool upsert(const QList<Media> &media, QString &error) override;

private:
    QSqlDatabase database_;
    QString upsertQuery_;
};

#endif // HAIKENANIME_SQLITEMEDIAREPOSITORY_H
