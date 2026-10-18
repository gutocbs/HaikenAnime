#include "GraphQlAniListDataSource.h"

#include "AniListGraphQlClient.h"
#include "AniListGraphQlUserListParser.h"
#include "GraphQlQueryStore.h"
#include "../../application/anilist/IAniListAuthProvider.h"

#include <QJsonObject>

GraphQlAniListDataSource::GraphQlAniListDataSource(AniListGraphQlClient &client,
                                                   GraphQlQueryStore &queryStore,
                                                   IAniListAuthProvider *authProvider,
                                                   const int userListPerChunk)
    : client_(client), queryStore_(queryStore), authProvider_(authProvider),
      userListPerChunk_(qBound(1, userListPerChunk, 500)) {
}

bool GraphQlAniListDataSource::fetchPage(const AniListDataSourceRequest &request,
                                         AniListDataSourceResult &result, QString &error) {
    result = {};
    error.clear();
    const auto partition = request.filter.partition;
    if (partition != SyncPartition::UserList) {
        error = QStringLiteral("AniList source does not support partition %1.").arg(ToString(partition));
        return false;
    }
    if (!authProvider_ || authProvider_->credentials().username.isEmpty()) {
        error = QStringLiteral("AniList user-list refresh requires an authenticated identity.");
        return false;
    }
    QString query;
    if (!queryStore_.load(query, error)) {
        return false;
    }

    const auto requestedType = request.filter.type.trimmed().isEmpty()
        ? QStringLiteral("ANIME") : request.filter.type.trimmed().toUpper();
    const QJsonObject variables{{QStringLiteral("userName"), authProvider_->credentials().username},
                                {QStringLiteral("chunk"), qMax(1, request.filter.startingPage)},
                                {QStringLiteral("perChunk"), userListPerChunk_},
                                {QStringLiteral("type"), requestedType}};

    AniListGraphQlResponse response;
    if (!client_.execute(query, variables, response, error)) {
        return false;
    }
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }

    result.completedPartition = partition;
    const auto collection = response.data.value(QStringLiteral("MediaListCollection"));
    if (!collection.isObject()) {
        error = QStringLiteral("AniList user-list response does not contain MediaListCollection.");
        return false;
    }
    const auto hasNextChunk = collection.toObject().value(QStringLiteral("hasNextChunk"));
    if (!hasNextChunk.isBool()) {
        error = QStringLiteral("AniList MediaListCollection contains invalid hasNextChunk pagination metadata.");
        return false;
    }
    QList<MediaType> acceptedMediaTypes;
    if (requestedType == QStringLiteral("ANIME")) {
        acceptedMediaTypes.append(MediaType::Anime);
    } else if (requestedType == QStringLiteral("MANGA")) {
        acceptedMediaTypes = {MediaType::Manga, MediaType::Novel};
    }
    if (!AniListGraphQlUserListParser::parse(response.data, result.page.media, error,
                                              request.filter.acceptedListStatuses,
                                              acceptedMediaTypes, &result.observedMediaIds)) return false;
    result.page.currentPage = qMax(1, request.filter.startingPage);
    result.page.hasNextPage = hasNextChunk.toBool();
    result.page.totalPages = result.page.hasNextPage ? result.page.currentPage + 1 : result.page.currentPage;
    result.isCompleteAuthoritativeSnapshot = request.filter.startingPage == 1
        && request.filter.status.isEmpty() && request.filter.list.isEmpty();
    return true;
}
