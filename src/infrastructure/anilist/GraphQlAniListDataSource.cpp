#include "GraphQlAniListDataSource.h"

#include "AniListGraphQlClient.h"
#include "AniListGraphQlPageParser.h"
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

namespace {
QJsonValue variableValue(const QString &value) {
    return value.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(value);
}

bool isCatalogPartition(const SyncPartition partition) {
    return partition == SyncPartition::ActiveCatalog
        || partition == SyncPartition::InactiveCatalog
        || partition == SyncPartition::CompletedCatalog;
}
}

bool GraphQlAniListDataSource::fetchPage(const AniListDataSourceRequest &request,
                                         AniListDataSourceResult &result, QString &error) {
    result = {};
    error.clear();
    const auto partition = request.filter.partition;
    if (partition == SyncPartition::UserList
        && (!authProvider_ || authProvider_->credentials().username.isEmpty())) {
        error = QStringLiteral("AniList user-list refresh requires an authenticated identity.");
        return false;
    }
    if (partition != SyncPartition::UserList && !isCatalogPartition(partition)) {
        error = QStringLiteral("AniList source does not support partition %1.").arg(ToString(partition));
        return false;
    }
    QString query;
    if (!queryStore_.load(query, error)) {
        return false;
    }

    AniListDataSourceRequest networkRequest = request;
    networkRequest.variables = {};
    if (partition == SyncPartition::UserList) {
        const auto credentials = authProvider_->credentials();
        const auto requestedType = request.filter.type.trimmed().isEmpty()
            ? QStringLiteral("ANIME") : request.filter.type.trimmed().toUpper();
        networkRequest.variables.insert(QStringLiteral("userName"), credentials.username);
        networkRequest.variables.insert(QStringLiteral("chunk"), qMax(1, request.filter.startingPage));
        networkRequest.variables.insert(QStringLiteral("perChunk"), userListPerChunk_);
        networkRequest.variables.insert(QStringLiteral("type"), requestedType);
    } else {
        networkRequest.variables.insert(QStringLiteral("page"), qMax(1, request.filter.startingPage));
        networkRequest.variables.insert(QStringLiteral("perPage"), qMax(1, request.filter.perPage));
        networkRequest.variables.insert(QStringLiteral("type"), variableValue(request.filter.type));
        networkRequest.variables.insert(QStringLiteral("status"), variableValue(request.filter.status));
        networkRequest.variables.insert(QStringLiteral("list"), variableValue(request.filter.list));
        networkRequest.variables.insert(QStringLiteral("userName"), variableValue(request.filter.username));
        networkRequest.variables.insert(QStringLiteral("includeCatalog"), true);
        networkRequest.variables.insert(QStringLiteral("includeUserList"), false);
    }

    AniListGraphQlResponse response;
    if (!client_.execute(query, networkRequest, response, error)) {
        return false;
    }
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }

    result.completedPartition = partition;
    if (partition == SyncPartition::UserList) {
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
        const auto requestedType = request.filter.type.trimmed().isEmpty()
            ? QStringLiteral("ANIME") : request.filter.type.trimmed().toUpper();
        if (requestedType == QStringLiteral("ANIME")) {
            acceptedMediaTypes.append(MediaType::Anime);
        } else if (requestedType == QStringLiteral("MANGA")) {
            acceptedMediaTypes = {MediaType::Manga, MediaType::Novel};
        }
        if (!AniListGraphQlUserListParser::parse(response.data, result.page.media, error,
                                                  request.filter.acceptedListStatuses,
                                                  acceptedMediaTypes)) return false;
        result.page.currentPage = qMax(1, request.filter.startingPage);
        result.page.hasNextPage = hasNextChunk.toBool();
        result.page.totalPages = result.page.hasNextPage ? result.page.currentPage + 1 : result.page.currentPage;
        result.isCompleteAuthoritativeSnapshot = request.filter.startingPage == 1
            && request.filter.type.isEmpty() && request.filter.status.isEmpty()
            && request.filter.list.isEmpty() && request.filter.acceptedListStatuses.isEmpty();
        return true;
    }

    if (!response.data.value(QStringLiteral("Page")).isObject()) {
        error = QStringLiteral("AniList catalog response does not contain Page.");
        return false;
    }
    if (!AniListGraphQlPageParser::parse(response.data, result.page, error)) return false;
    return true;
}
