#ifndef HAIKENANIME_IANILISTDATASOURCE_H
#define HAIKENANIME_IANILISTDATASOURCE_H

#include <QString>

#include "../../domain/anilist/AniListPage.h"

class IAniListDataSource {
public:
    virtual ~IAniListDataSource() = default;

    [[nodiscard]] virtual bool fetchPage(int page, int perPage, AniListPage &result,
                                         QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTDATASOURCE_H
