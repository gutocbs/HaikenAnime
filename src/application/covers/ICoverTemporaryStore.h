#ifndef HAIKENANIME_ICOVERTEMPORARYSTORE_H
#define HAIKENANIME_ICOVERTEMPORARYSTORE_H

#include <QString>

class ICoverTemporaryStore {
public:
    virtual ~ICoverTemporaryStore() = default;
    virtual bool ClearAbandoned(int &removedFiles, QString &error) const = 0;
};

#endif // HAIKENANIME_ICOVERTEMPORARYSTORE_H
