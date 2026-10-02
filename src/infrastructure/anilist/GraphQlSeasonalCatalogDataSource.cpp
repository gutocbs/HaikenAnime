#include "GraphQlSeasonalCatalogDataSource.h"

#include "AniListGraphQlClient.h"
#include "AniListGraphQlPageParser.h"
#include "GraphQlQueryStore.h"

#include <QJsonObject>

GraphQlSeasonalCatalogDataSource::GraphQlSeasonalCatalogDataSource(AniListGraphQlClient &client,
                                                                   GraphQlQueryStore &queryStore)
    : client_(client), queryStore_(queryStore) {
}

QJsonObject seasonalCatalogGraphQlVariables(const SeasonalCatalogRequest &request) {
    return {{QStringLiteral("year"), request.year},
            {QStringLiteral("season"), request.seasonKey},
            {QStringLiteral("page"), request.page},
            {QStringLiteral("perPage"), request.perPage}};
}

bool GraphQlSeasonalCatalogDataSource::Fetch(const SeasonalCatalogRequest &request,
                                             MediaPage &result, QString &error) {
    result = {};
    error.clear();
    if (!request.isValid()) {
        error = QStringLiteral("AniList seasonal catalog request is invalid.");
        return false;
    }

    QString query;
    if (!queryStore_.load(query, error)) return false;

    const auto variables = seasonalCatalogGraphQlVariables(request);
    AniListGraphQlResponse response;
    if (!client_.execute(query, variables, response, error)) return false;
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }
    return AniListGraphQlPageParser::parse(response.data, result, error);
}
