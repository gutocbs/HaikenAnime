#ifndef HAIKENANIME_RECORDEDGRAPHQLANILISTDATASOURCE_H
#define HAIKENANIME_RECORDEDGRAPHQLANILISTDATASOURCE_H

#include "../../application/anilist/IAniListDataSource.h"

#include "AniListGraphQlPageParser.h"
#include "AniListGraphQlResponseParser.h"
#include "AniListGraphQlUserListParser.h"

#include <QFile>
#include <QHash>
#include <QJsonDocument>

#include <utility>

class RecordedGraphQlAniListDataSource final : public IAniListDataSource {
public:
    explicit RecordedGraphQlAniListDataSource(QString filePath);
    bool fetchPage(const AniListDataSourceRequest &request, AniListDataSourceResult &result,
                   QString &error) override;
    [[nodiscard]] int fixtureReadCount() const { return fixtureReadCount_; }
    [[nodiscard]] int externalCallCount() const { return 0; }

private:
    [[nodiscard]] QString cacheKey(const AniListDataSourceRequest &request) const;

    QString filePath_;
    QHash<QString, AniListDataSourceResult> cachedResults_;
    int fixtureReadCount_ = 0;
};

inline RecordedGraphQlAniListDataSource::RecordedGraphQlAniListDataSource(QString filePath)
    : filePath_(std::move(filePath)) {
}

inline QString RecordedGraphQlAniListDataSource::cacheKey(const AniListDataSourceRequest &request) const {
    return request.queryIdentity + QLatin1Char('|') + ToString(request.filter.partition)
        + QLatin1Char('|') + QString::fromUtf8(QJsonDocument(request.variables).toJson(QJsonDocument::Compact));
}

inline bool RecordedGraphQlAniListDataSource::fetchPage(const AniListDataSourceRequest &request,
                                                        AniListDataSourceResult &result, QString &error) {
    result = {};
    error.clear();

    const auto key = cacheKey(request);
    if (!request.refresh && cachedResults_.contains(key)) {
        result = cachedResults_.value(key);
        return true;
    }

    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("Could not open recorded AniList GraphQL response: %1")
                    .arg(file.errorString());
        return false;
    }
    ++fixtureReadCount_;

    AniListGraphQlResponse response;
    if (!AniListGraphQlResponseParser::parse(file.readAll(), response, error)) return false;
    if (response.hasErrors()) {
        error = response.errors.first().message;
        return false;
    }
    if (request.filter.partition == SyncPartition::UserList) {
        if (response.data.value(QStringLiteral("MediaListCollection")).isObject()) {
            if (!AniListGraphQlUserListParser::parse(response.data, result.page.media, error)) return false;
            result.page.currentPage = qMax(1, request.filter.startingPage);
            result.page.hasNextPage = response.data.value(QStringLiteral("MediaListCollection")).toObject()
                                          .value(QStringLiteral("hasNextChunk")).toBool();
            result.page.totalPages = result.page.hasNextPage ? result.page.currentPage + 1 : result.page.currentPage;
        } else if (response.data.value(QStringLiteral("Page")).isObject()) {
            if (!AniListGraphQlPageParser::parse(response.data, result.page, error)) return false;
        } else {
            error = QStringLiteral("Recorded AniList user-list response contains neither MediaListCollection nor Page.");
            return false;
        }
    } else {
        if (!response.data.value(QStringLiteral("Page")).isObject()) {
            error = QStringLiteral("Recorded AniList catalog response does not contain Page.");
            return false;
        }
        if (!AniListGraphQlPageParser::parse(response.data, result.page, error)) return false;
    }
    if (result.page.currentPage != qMax(1, request.filter.startingPage)) {
        error = QStringLiteral("Recorded AniList response contains page %1 while page %2 was requested.")
                    .arg(result.page.currentPage)
                    .arg(qMax(1, request.filter.startingPage));
        return false;
    }

    result.completedPartition = request.filter.partition;
    result.isCompleteAuthoritativeSnapshot = request.filter.partition == SyncPartition::UserList
        && request.filter.startingPage == 1 && request.filter.type.isEmpty()
        && request.filter.status.isEmpty() && request.filter.list.isEmpty() && !result.page.hasNextPage;
    cachedResults_.insert(key, result);
    return true;
}

#endif
