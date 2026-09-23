#ifndef HAIKENANIME_SQLITEMEDIAREPOSITORY_H
#define HAIKENANIME_SQLITEMEDIAREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/anilist/IMediaRepository.h"

/** Persists AniList media in the application's SQLite database. */
class SqliteMediaRepository final : public IMediaRepository {
public:
    /** Creates a repository using an opened database connection and external upsert SQL. */
    SqliteMediaRepository(QSqlDatabase database, QString upsertQuery, QString readQuery);

    /** Upserts external media in one transaction while preserving local user fields. */
    [[nodiscard]] bool Upsert(const QList<Media> &media, QString &error) override;

    /** Reads persisted media using the supplied ordered SELECT query. */
    [[nodiscard]] QList<Media> ReadAll(QString &error) override;

private:
    QSqlDatabase database_;
    QString upsertQuery_;
    QString readQuery_;
};

#endif // HAIKENANIME_SQLITEMEDIAREPOSITORY_H
