#ifndef HAIKENANIME_GRAPHQLANILISTDATASOURCE_H
#define HAIKENANIME_GRAPHQLANILISTDATASOURCE_H

#include <QString>

#include "../../application/media/IMediaDataSource.h"

class AniListGraphQlClient;
class GraphQlQueryStore;

/** Loads paged AniList media through the GraphQL client and an external query file. */
class GraphQlAniListDataSource final : public IMediaDataSource {
public:
    /** Creates a source from an already configured client and query store. */
    GraphQlAniListDataSource(AniListGraphQlClient &client, GraphQlQueryStore &queryStore);

    /** Fetches one GraphQL page using the supplied filter. */
    [[nodiscard]] bool fetchPage(const MediaSyncFilter &filter, MediaPage &result,
                                 QString &error) override;

private:
    AniListGraphQlClient &client_;
    GraphQlQueryStore &queryStore_;
};

#endif // HAIKENANIME_GRAPHQLANILISTDATASOURCE_H
