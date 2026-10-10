#ifndef HAIKENANIME_GRAPHQLANILISTDATASOURCE_H
#define HAIKENANIME_GRAPHQLANILISTDATASOURCE_H

#include <QString>

#include "../../application/anilist/IAniListDataSource.h"

class AniListGraphQlClient;
class GraphQlQueryStore;
class IAniListAuthProvider;

/** Loads paged AniList media through the GraphQL client and an external query file. */
class GraphQlAniListDataSource final : public IAniListDataSource {
public:
    /** Creates a source from an already configured client and query store. */
    GraphQlAniListDataSource(AniListGraphQlClient &client, GraphQlQueryStore &queryStore,
                             IAniListAuthProvider *authProvider = nullptr,
                             int userListPerChunk = 100);

    /** Fetches the exact AniList partition requested through explicit GraphQL variables. */
    [[nodiscard]] bool fetchPage(const AniListDataSourceRequest &request,
                                 AniListDataSourceResult &result, QString &error) override;

private:
    AniListGraphQlClient &client_;
    GraphQlQueryStore &queryStore_;
    IAniListAuthProvider *authProvider_ = nullptr;
    int userListPerChunk_ = 100;
};

#endif // HAIKENANIME_GRAPHQLANILISTDATASOURCE_H
