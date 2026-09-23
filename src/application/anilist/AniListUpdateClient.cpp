#include "AniListUpdateClient.h"

#include "../../infrastructure/anilist/AniListGraphQlClient.h"
#include "../../infrastructure/anilist/GraphQlQueryStore.h"

#include <QJsonObject>

AniListUpdateClient::AniListUpdateClient(AniListGraphQlClient &graphQlClient,
                                         GraphQlQueryStore &progressQuery,
                                         GraphQlQueryStore &scoreQuery,
                                         GraphQlQueryStore &listStatusQuery,
                                         GraphQlQueryStore &deleteQuery)
    : graphQlClient_(graphQlClient), progressQuery_(progressQuery), scoreQuery_(scoreQuery),
      listStatusQuery_(listStatusQuery), deleteQuery_(deleteQuery) {
}

bool AniListUpdateClient::Execute(GraphQlQueryStore &queryStore, const QJsonObject &variables,
                                  QString &error) const {
    QString query;
    if (!queryStore.load(query, error)) {
        return false;
    }

    AniListGraphQlResponse response;
    if (!graphQlClient_.execute(query, variables, response, error)) {
        return false;
    }
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }
    return response.hasData();
}

bool AniListUpdateClient::UpdateProgress(const int mediaId, const int progress, QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId},
                          {QStringLiteral("progress"), progress}};
    return Execute(progressQuery_, variables, error);
}

bool AniListUpdateClient::UpdateScore(const int mediaId, const double score, QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId},
                          {QStringLiteral("score"), score}};
    return Execute(scoreQuery_, variables, error);
}

bool AniListUpdateClient::UpdateListStatus(const int mediaId, const QString &status,
                                           QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId},
                          {QStringLiteral("status"), status}};
    return Execute(listStatusQuery_, variables, error);
}

bool AniListUpdateClient::DeleteListEntry(const int mediaId, QString &error) {
    QJsonObject variables{{QStringLiteral("mediaId"), mediaId}};
    return Execute(deleteQuery_, variables, error);
}
