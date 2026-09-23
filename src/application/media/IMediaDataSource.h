#ifndef HAIKENANIME_IMEDIADATASOURCE_H
#define HAIKENANIME_IMEDIADATASOURCE_H

#include <QString>

#include "../../domain/media/MediaPage.h"
#include "../../domain/media/MediaSyncFilter.h"

class IMediaDataSource {
public:
    virtual ~IMediaDataSource() = default;

    [[nodiscard]] virtual bool fetchPage(const MediaSyncFilter &filter, MediaPage &result,
                                         QString &error) = 0;
};

#endif
