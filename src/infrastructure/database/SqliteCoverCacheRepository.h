#ifndef HAIKENANIME_SQLITECOVERCACHEREPOSITORY_H
#define HAIKENANIME_SQLITECOVERCACHEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/covers/ICoverCacheRepository.h"

class SqliteCoverCacheRepository final : public ICoverCacheRepository {
public:
    SqliteCoverCacheRepository(QSqlDatabase database, QString readQuery, QString upsertQuery,
                               QString deleteQuery, QString clearQuery);

    bool ReadAll(QHash<int, CoverCacheEntry> &entries, QString &error) override;
    bool Upsert(const CoverCacheEntry &entry, QString &error) override;
    bool Remove(int mediaId, QString &error) override;
    bool Clear(QString &error) override;

private:
    QSqlDatabase database_;
    QString readQuery_;
    QString upsertQuery_;
    QString deleteQuery_;
    QString clearQuery_;
};

#endif // HAIKENANIME_SQLITECOVERCACHEREPOSITORY_H
