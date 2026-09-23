#ifndef HAIKENANIME_FILEANILISTDATASOURCE_H
#define HAIKENANIME_FILEANILISTDATASOURCE_H

#include <QString>

#include "../../application/anilist/IAniListDataSource.h"

/** Reads AniList-compatible media data from a local JSON fixture. */
class FileAniListDataSource final : public IAniListDataSource {
public:
    /** Creates a source for the supplied fixture path. */
    explicit FileAniListDataSource(QString filePath);

    /** Reads, filters and paginates one fixture page. */
    [[nodiscard]] bool fetchPage(const AniListSyncFilter &filter, AniListPage &result,
                                 QString &error) override;

private:
    QString filePath_;
};

#endif // HAIKENANIME_FILEANILISTDATASOURCE_H
