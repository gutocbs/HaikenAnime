#ifndef HAIKENANIME_IANILISTDATASOURCE_H
#define HAIKENANIME_IANILISTDATASOURCE_H

#include <QSet>
#include <QString>

#include "../media/MediaPage.h"
#include "../media/MediaSyncFilter.h"

struct AniListDataSourceRequest final {
    MediaSyncFilter filter;

    [[nodiscard]] static AniListDataSourceRequest ForPartition(const SyncPartition partition) {
        AniListDataSourceRequest request;
        request.filter.partition = partition;
        return request;
    }

    void setPage(const int page) { filter.startingPage = page; }
};

struct AniListDataSourceResult final {
    MediaPage page;
    SyncPartition completedPartition = SyncPartition::UserList;
    bool isCompleteAuthoritativeSnapshot = false;
    /** Every media id present in the remote entries, before any status or type filter. */
    QSet<int> observedMediaIds;
};

/** Reads one AniList partition and reports whether its result is a complete snapshot. */
class IAniListDataSource {
public:
    virtual ~IAniListDataSource() = default;

    [[nodiscard]] virtual bool fetchPage(const AniListDataSourceRequest &request,
                                         AniListDataSourceResult &result, QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTDATASOURCE_H
