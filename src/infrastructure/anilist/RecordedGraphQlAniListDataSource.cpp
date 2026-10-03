#include "RecordedGraphQlAniListDataSource.h"

#include "AniListGraphQlPageParser.h"
#include "AniListGraphQlResponseParser.h"
#include "AniListGraphQlUserListParser.h"

#include <QFile>

#include <utility>

RecordedGraphQlAniListDataSource::RecordedGraphQlAniListDataSource(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool RecordedGraphQlAniListDataSource::fetchPage(const MediaSyncFilter &filter,
                                                 MediaPage &result, QString &error) {
    result = {};
    error.clear();

    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Could not open recorded AniList GraphQL response: %1")
                    .arg(file.errorString());
        return false;
    }

    AniListGraphQlResponse response;
    if (!AniListGraphQlResponseParser::parse(file.readAll(), response, error)) {
        return false;
    }
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }
    if (response.data.value(QStringLiteral("MediaListCollection")).isObject()) {
        if (!AniListGraphQlUserListParser::parse(response.data, result.media, error)) return false;
        result.currentPage = 1;
        result.totalPages = 1;
        result.hasNextPage = false;
    } else if (!AniListGraphQlPageParser::parse(response.data, result, error)) return false;
    if (result.currentPage != qMax(1, filter.startingPage)) {
        error = QStringLiteral("Recorded AniList response contains page %1 while page %2 was requested.")
                    .arg(result.currentPage)
                    .arg(qMax(1, filter.startingPage));
        return false;
    }

    return true;
}
