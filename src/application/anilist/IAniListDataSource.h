#ifndef HAIKENANIME_IANILISTDATASOURCE_H
#define HAIKENANIME_IANILISTDATASOURCE_H

#include <QString>

#include "../../domain/anilist/AniListPage.h"
#include "../../domain/anilist/AniListSyncFilter.h"

class IAniListDataSource {
public:
    virtual ~IAniListDataSource() = default;

    /** Fetches one page using the filter pagination values and reports failures through error. */
    [[nodiscard]] virtual bool fetchPage(const AniListSyncFilter &filter, AniListPage &result,
                                         QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTDATASOURCE_H
