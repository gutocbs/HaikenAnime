#ifndef HAIKENANIME_ANILISTVIEWERCLIENT_H
#define HAIKENANIME_ANILISTVIEWERCLIENT_H

#include "../../application/anilist/IAniListViewerClient.h"

class AniListGraphQlClient;
class GraphQlQueryStore;

class AniListViewerClient final : public IAniListViewerClient {
public:
    AniListViewerClient(AniListGraphQlClient &client, GraphQlQueryStore &queryStore);
    bool loadViewer(AniListViewer &viewer, QString &error) override;
private:
    AniListGraphQlClient &client_;
    GraphQlQueryStore &queryStore_;
};

#endif
