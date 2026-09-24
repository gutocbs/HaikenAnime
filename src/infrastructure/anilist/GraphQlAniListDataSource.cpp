#include "GraphQlAniListDataSource.h"

#include "AniListGraphQlClient.h"
#include "AniListMediaMapper.h"
#include "GraphQlQueryStore.h"

#include <QJsonArray>
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

    const auto pageObject = response.data.value(QStringLiteral("Page")).toObject();
    const auto pageInfo = pageObject.value(QStringLiteral("pageInfo")).toObject();
    result.currentPage = pageInfo.value(QStringLiteral("currentPage")).toInt();
    result.totalPages = pageInfo.value(QStringLiteral("lastPage")).toInt();
    result.hasNextPage = pageInfo.value(QStringLiteral("hasNextPage")).toBool();

    for (const auto &value : pageObject.value(QStringLiteral("media")).toArray()) {
        const auto object = value.toObject();
        result.media.append(AniListMediaMapper::ToDomainMedia(
            AniListMediaMapper::FromGraphQlJson(object)));
    }

    return true;
}
