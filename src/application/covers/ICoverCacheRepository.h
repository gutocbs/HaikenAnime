#ifndef HAIKENANIME_ICOVERCACHEREPOSITORY_H
#define HAIKENANIME_ICOVERCACHEREPOSITORY_H

#include <QHash>
#include <QString>

#include "CoverCacheEntry.h"

class ICoverCacheRepository {
public:
    virtual ~ICoverCacheRepository() = default;
    virtual bool ReadAll(QHash<int, CoverCacheEntry> &entries, QString &error) = 0;
    virtual bool Upsert(const CoverCacheEntry &entry, QString &error) = 0;
    virtual bool Remove(int mediaId, QString &error) = 0;
    virtual bool Clear(QString &error) = 0;
};

#endif // HAIKENANIME_ICOVERCACHEREPOSITORY_H
