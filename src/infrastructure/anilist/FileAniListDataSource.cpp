#include "FileAniListDataSource.h"

#include "AniListMediaMapper.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace {

bool matchesFilter(const QJsonObject &object, const MediaSyncFilter &filter) {
    const auto format = object.value(QStringLiteral("format")).toString();
    const auto status = object.value(QStringLiteral("status")).toString();
    const auto list = object.value(QStringLiteral("list")).toString();

    return (filter.type.isEmpty() || format.compare(filter.type, Qt::CaseInsensitive) == 0)
        && (filter.status.isEmpty() || status.compare(filter.status, Qt::CaseInsensitive) == 0)
        && (filter.list.isEmpty() || list.compare(filter.list, Qt::CaseInsensitive) == 0);
}

}

FileAniListDataSource::FileAniListDataSource(QString filePath)
    : filePath_(std::move(filePath)) {
}

bool FileAniListDataSource::fetchPage(const MediaSyncFilter &filter, MediaPage &result,
                                      QString &error) {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Could not open AniList fixture: %1").arg(file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        error = QStringLiteral("Invalid AniList fixture: %1").arg(parseError.errorString());
        return false;
    }

    QList<QJsonObject> filtered;
    for (const auto &value : document.array()) {
        if (!value.isObject()) {
            continue;
        }
        const auto object = value.toObject();
        if (!matchesFilter(object, filter)) {
            continue;
        }
        filtered.append(object);
    }

    const int page = qMax(1, filter.startingPage);
    const int perPage = qMax(1, filter.perPage);
    const int offset = (page - 1) * perPage;
    result = {};
    result.currentPage = page;
    result.totalPages = (filtered.size() + perPage - 1) / perPage;
    result.hasNextPage = page < result.totalPages;

    for (int index = offset; index < qMin(offset + perPage, filtered.size()); ++index) {
        const auto object = filtered.at(index);
            result.media.append(AniListMediaMapper::ToDomainMedia(
                AniListMediaMapper::fromFixtureJson(object)));
    }

    return true;
}
