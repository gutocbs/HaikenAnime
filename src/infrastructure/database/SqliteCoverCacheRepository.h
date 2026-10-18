#ifndef HAIKENANIME_SQLITECOVERCACHEREPOSITORY_H
#define HAIKENANIME_SQLITECOVERCACHEREPOSITORY_H

#include <QSqlDatabase>

#include "../../application/covers/ICoverCacheRepository.h"

class SqliteCoverCacheRepository final : public ICoverCacheRepository {
public:
    explicit SqliteCoverCacheRepository(QSqlDatabase database);

    bool ReadAll(QHash<int, CoverCacheEntry> &entries, QString &error) override;
    bool Upsert(const CoverCacheEntry &entry, QString &error) override;
    bool Remove(int mediaId, QString &error) override;
    bool Clear(int &removedEntries, QString &error) override;

private:
    QSqlDatabase database_;
};

#endif // HAIKENANIME_SQLITECOVERCACHEREPOSITORY_H
