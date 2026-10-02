#ifndef HAIKENANIME_GRAPHQLSEASONALCATALOGDATASOURCE_H
#define HAIKENANIME_GRAPHQLSEASONALCATALOGDATASOURCE_H

#include "../../application/catalog/ISeasonalCatalogDataSource.h"

#include <QJsonObject>

class AniListGraphQlClient;
class GraphQlQueryStore;

/** Builds the explicit variables sent to the dedicated seasonal query. */
[[nodiscard]] QJsonObject seasonalCatalogGraphQlVariables(const SeasonalCatalogRequest &request);

/** Loads explicit AniList season pages through a dedicated external GraphQL operation. */
class GraphQlSeasonalCatalogDataSource final : public ISeasonalCatalogDataSource {
public:
    GraphQlSeasonalCatalogDataSource(AniListGraphQlClient &client, GraphQlQueryStore &queryStore);

    [[nodiscard]] bool Fetch(const SeasonalCatalogRequest &request, MediaPage &result,
                             QString &error) override;

private:
    AniListGraphQlClient &client_;
    GraphQlQueryStore &queryStore_;
};

#endif // HAIKENANIME_GRAPHQLSEASONALCATALOGDATASOURCE_H
