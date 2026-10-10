#ifndef HAIKENANIME_ANILISTUPDATECLIENT_H
#define HAIKENANIME_ANILISTUPDATECLIENT_H

#include <QJsonObject>

#include "../../application/anilist/IAniListUpdateClient.h"

class AniListGraphQlClient;
class GraphQlQueryStore;

/** Implements AniList mutations using external GraphQL query files. */
class AniListUpdateClient final : public IAniListUpdateClient {
public:
    /** Creates an update client that consolidates each media update into one mutation. */
    AniListUpdateClient(AniListGraphQlClient &graphQlClient,
                        GraphQlQueryStore &updateQuery);

    /** Sends all changes for one media item through the update operation. */
    [[nodiscard]] bool updateMedia(const AniListMediaPendingChanges &changes,
                                   QString &error) override;

private:
    [[nodiscard]] bool Execute(GraphQlQueryStore &queryStore, const QJsonObject &variables,
                               QString &error) const;

    AniListGraphQlClient &graphQlClient_;
    GraphQlQueryStore &updateQuery_;
};

#endif // HAIKENANIME_ANILISTUPDATECLIENT_H
