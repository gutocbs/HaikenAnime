#include "AniListGraphQlPageParser.h"

#include "AniListMediaMapper.h"

#include <QJsonArray>

namespace {
bool isInteger(const QJsonValue &value) {
    return value.isDouble() && value.toDouble() == value.toInt();
}
}

bool AniListGraphQlPageParser::parse(const QJsonObject &data, MediaPage &page, QString &error) {
    page = {};
    error.clear();

    const auto pageValue = data.value(QStringLiteral("Page"));
    if (!pageValue.isObject()) {
        error = QStringLiteral("AniList response is missing the Page object.");
        return false;
    }

    const auto pageObject = pageValue.toObject();
    const auto pageInfoValue = pageObject.value(QStringLiteral("pageInfo"));
    const auto mediaValue = pageObject.value(QStringLiteral("media"));
    if (!pageInfoValue.isObject() || !mediaValue.isArray()) {
        error = QStringLiteral("AniList Page has an invalid pageInfo or media field.");
        return false;
    }

    const auto pageInfo = pageInfoValue.toObject();
    const auto currentPage = pageInfo.value(QStringLiteral("currentPage"));
    const auto lastPage = pageInfo.value(QStringLiteral("lastPage"));
    const auto hasNextPage = pageInfo.value(QStringLiteral("hasNextPage"));
    if (!isInteger(currentPage) || currentPage.toInt() < 1
        || !isInteger(lastPage) || lastPage.toInt() < currentPage.toInt()
        || !hasNextPage.isBool()) {
        error = QStringLiteral("AniList Page contains invalid pagination metadata.");
        return false;
    }

    QList<Media> media;
    for (const auto &value : mediaValue.toArray()) {
        if (!value.isObject()) {
            error = QStringLiteral("AniList Page contains a malformed media entry.");
            return false;
        }
        media.append(AniListMediaMapper::ToDomainMedia(
            AniListMediaMapper::FromGraphQlJson(value.toObject())));
    }

    page.media = std::move(media);
    page.currentPage = currentPage.toInt();
    page.totalPages = lastPage.toInt();
    page.hasNextPage = hasNextPage.toBool();
    return true;
}
