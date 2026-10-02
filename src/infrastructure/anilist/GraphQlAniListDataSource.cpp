#include "GraphQlAniListDataSource.h"

#include "AniListGraphQlClient.h"
#include "AniListGraphQlPageParser.h"
#include "GraphQlQueryStore.h"

#include <QJsonObject>

GraphQlAniListDataSource::GraphQlAniListDataSource(AniListGraphQlClient &client,
                                                   GraphQlQueryStore &queryStore)
    : client_(client), queryStore_(queryStore) {
}

bool GraphQlAniListDataSource::fetchPage(const MediaSyncFilter &filter, MediaPage &result,
                                         QString &error) {
    result = {};
    error.clear();
    QString query;
    if (!queryStore_.load(query, error)) {
        return false;
    }

    QJsonObject variables;
    variables.insert(QStringLiteral("page"), qMax(1, filter.startingPage));
    variables.insert(QStringLiteral("perPage"), qMax(1, filter.perPage));

    AniListGraphQlResponse response;
    if (!client_.execute(query, variables, response, error)) {
        return false;
    }
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }

    return AniListGraphQlPageParser::parse(response.data, result, error);
}
