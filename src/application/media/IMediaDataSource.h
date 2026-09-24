#ifndef HAIKENANIME_IMEDIADATASOURCE_H
#define HAIKENANIME_IMEDIADATASOURCE_H

#include <QString>

#include "MediaPage.h"
#include "MediaSyncFilter.h"

class IMediaDataSource {
public:
    virtual ~IMediaDataSource() = default;

    /** Replaces result with one page and clears error on success. */
    [[nodiscard]] virtual bool fetchPage(const MediaSyncFilter &filter, MediaPage &result,
                                         QString &error) = 0;
};

#endif
