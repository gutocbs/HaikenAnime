#ifndef HAIKENANIME_ANILISTUPDATECLIENT_H
#define HAIKENANIME_ANILISTUPDATECLIENT_H

#include <QJsonObject>

#include "IAniListUpdateClient.h"

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

    /** Sends the progress mutation. */
    [[nodiscard]] bool UpdateProgress(int mediaId, int progress, QString &error) override;

    /** Sends the score mutation. */
    [[nodiscard]] bool UpdateScore(int mediaId, double score, QString &error) override;

    /** Sends the list-status mutation. */
    [[nodiscard]] bool UpdateListStatus(int mediaId, const QString &status,
                                        QString &error) override;

    /** Sends the delete mutation after confirmation. */
    [[nodiscard]] bool DeleteListEntry(int mediaId, QString &error) override;

private:
    [[nodiscard]] bool Execute(GraphQlQueryStore &queryStore, const QJsonObject &variables,
                               QString &error) const;

    AniListGraphQlClient &graphQlClient_;
    GraphQlQueryStore &progressQuery_;
    GraphQlQueryStore &scoreQuery_;
    GraphQlQueryStore &listStatusQuery_;
    GraphQlQueryStore &deleteQuery_;
};

#endif // HAIKENANIME_ANILISTUPDATECLIENT_H
