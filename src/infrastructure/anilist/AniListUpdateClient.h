#ifndef HAIKENANIME_ANILISTUPDATECLIENT_H
#define HAIKENANIME_ANILISTUPDATECLIENT_H

#include <QJsonObject>

#include "../../application/anilist/IAniListUpdateClient.h"

class AniListGraphQlClient;
class GraphQlQueryStore;

/** Implements AniList mutations using external GraphQL query files. */
class AniListUpdateClient final : public IAniListUpdateClient {
public:
    /** Creates an update client with one query store for each mutation. */
    AniListUpdateClient(AniListGraphQlClient &graphQlClient,
                        GraphQlQueryStore &progressQuery,
                        GraphQlQueryStore &scoreQuery,
                        GraphQlQueryStore &listStatusQuery,
                        GraphQlQueryStore &deleteQuery);

    /** Sends all changes for one media item through the update operation. */
    [[nodiscard]] bool UpdateMedia(const AniListMediaPendingChanges &changes,
                                   QString &error) override;

private:
    [[nodiscard]] bool UpdateProgress(int mediaId, int progress, QString &error);
    [[nodiscard]] bool UpdateScore(int mediaId, double score, QString &error);
    [[nodiscard]] bool UpdateListStatus(int mediaId, const QString &status, QString &error);
    [[nodiscard]] bool DeleteListEntry(int mediaId, QString &error);
    [[nodiscard]] bool Execute(GraphQlQueryStore &queryStore, const QJsonObject &variables,
                               QString &error) const;

    AniListGraphQlClient &graphQlClient_;
    GraphQlQueryStore &progressQuery_;
    GraphQlQueryStore &scoreQuery_;
    GraphQlQueryStore &listStatusQuery_;
    GraphQlQueryStore &deleteQuery_;
};

#endif // HAIKENANIME_ANILISTUPDATECLIENT_H
