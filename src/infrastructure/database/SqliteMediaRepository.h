#ifndef HAIKENANIME_SQLITEMEDIAREPOSITORY_H
#define HAIKENANIME_SQLITEMEDIAREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/media/IMediaRepository.h"
class AsyncLogger;

/** Persists AniList media in the application's SQLite database. */
class SqliteMediaRepository final : public IMediaRepository {
public:
    /** Creates a repository using an opened database connection and external upsert SQL. */
    SqliteMediaRepository(QSqlDatabase database, QString upsertQuery, QString readQuery,
                          QString readActiveMediaIdsQuery, QString markSourceRemovedQuery);
    void setLogger(AsyncLogger *logger);

    /** Upserts external media in one transaction while preserving local user fields. */
    [[nodiscard]] bool upsert(const QList<Media> &media, QString &error) override;

    /** Reads persisted media using the supplied ordered SELECT query. */
    [[nodiscard]] bool readAll(QList<Media> &media, QString &error) override;
    [[nodiscard]] bool reconcileAuthoritativeSnapshot(
        const QSet<int> &observedMediaIds, int &removedCount, QString &error) override;

private:
    QSqlDatabase database_;
    QString upsertQuery_;
    QString readQuery_;
    QString readActiveMediaIdsQuery_;
    QString markSourceRemovedQuery_;
    AsyncLogger *logger_ = nullptr;
};

#endif // HAIKENANIME_SQLITEMEDIAREPOSITORY_H
