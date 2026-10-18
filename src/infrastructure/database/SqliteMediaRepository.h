#ifndef HAIKENANIME_SQLITEMEDIAREPOSITORY_H
#define HAIKENANIME_SQLITEMEDIAREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/media/IMediaRepository.h"
class AsyncLogger;

/** Persists AniList media in the application's SQLite database. */
class SqliteMediaRepository final : public IMediaRepository {
public:
    /** Creates a repository over an opened connection; SQL comes from SqliteQueries. */
    explicit SqliteMediaRepository(QSqlDatabase database);
    void setLogger(AsyncLogger *logger);

    /** Upserts external media in one transaction while preserving local user fields. */
    [[nodiscard]] bool upsert(const QList<Media> &media, QString &error) override;

    /** Reads persisted media using the supplied ordered SELECT query. */
    [[nodiscard]] bool readAll(QList<Media> &media, QString &error) override;
    [[nodiscard]] bool reconcileAuthoritativeSnapshot(
        const QSet<int> &observedMediaIds, const QSet<int> &excludedMediaIds,
        int &removedCount, QString &error) override;

private:
    QSqlDatabase database_;
    AsyncLogger *logger_ = nullptr;
};

#endif // HAIKENANIME_SQLITEMEDIAREPOSITORY_H
