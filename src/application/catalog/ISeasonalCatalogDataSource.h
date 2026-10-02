#ifndef HAIKENANIME_ISEASONALCATALOGDATASOURCE_H
#define HAIKENANIME_ISEASONALCATALOGDATASOURCE_H

#include "SeasonalCatalogTypes.h"
#include "../media/MediaPage.h"

#include <QString>

/** Reads one externally paged AniList seasonal catalog without user-list filtering. */
class ISeasonalCatalogDataSource {
public:
    virtual ~ISeasonalCatalogDataSource() = default;

    [[nodiscard]] virtual bool Fetch(const SeasonalCatalogRequest &request, MediaPage &result,
                                     QString &error) = 0;
};

#endif // HAIKENANIME_ISEASONALCATALOGDATASOURCE_H
