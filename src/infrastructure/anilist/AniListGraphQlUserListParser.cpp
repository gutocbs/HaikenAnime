#include "AniListGraphQlUserListParser.h"

#include "AniListMediaMapper.h"
#include <QJsonArray>
#include <QSet>

bool AniListGraphQlUserListParser::parse(const QJsonObject &data, QList<Media> &media, QString &error,
                                         const QStringList &acceptedListStatuses,
                                         const QList<MediaType> &acceptedMediaTypes) {
    media.clear();
    error.clear();
    const auto collection = data.value(QStringLiteral("MediaListCollection"));
    if (!collection.isObject()) {
        error = QStringLiteral("AniList response is missing the MediaListCollection object.");
        return false;
    }
    const auto lists = collection.toObject().value(QStringLiteral("lists"));
    if (!lists.isArray()) {
        error = QStringLiteral("AniList MediaListCollection has an invalid lists field.");
        return false;
    }
    QSet<QString> acceptedStatuses;
    for (const auto &status : acceptedListStatuses) {
        acceptedStatuses.insert(status.trimmed().toUpper());
    }
    QSet<int> seenIds;
    for (const auto &listValue : lists.toArray()) {
        if (!listValue.isObject()) {
            error = QStringLiteral("AniList MediaListCollection contains a malformed list.");
            return false;
        }
        const auto entries = listValue.toObject().value(QStringLiteral("entries"));
        if (!entries.isArray()) {
            error = QStringLiteral("AniList media list contains an invalid entries field.");
            return false;
        }
        for (const auto &entryValue : entries.toArray()) {
            if (!entryValue.isObject()) {
                error = QStringLiteral("AniList media list contains a malformed entry.");
                return false;
            }
            const auto entry = entryValue.toObject();
            const auto entryStatus = entry.value(QStringLiteral("status")).toString().toUpper();
            if (!acceptedStatuses.isEmpty() && !acceptedStatuses.contains(entryStatus)) {
                continue;
            }
            const auto mediaValue = entry.value(QStringLiteral("media"));
            if (!mediaValue.isObject()) {
                continue;
            }
            auto mediaObject = mediaValue.toObject();
            mediaObject.insert(QStringLiteral("userListStatus"), entry.value(QStringLiteral("status")));
            mediaObject.insert(QStringLiteral("progress"), entry.value(QStringLiteral("progress")));
            mediaObject.insert(QStringLiteral("score"), entry.value(QStringLiteral("score")));
            const auto id = mediaObject.value(QStringLiteral("id")).toInt();
            if (id <= 0 || seenIds.contains(id)) {
                continue;
            }
            const auto mappedMedia = AniListMediaMapper::ToDomainMedia(
                AniListMediaMapper::FromGraphQlJson(mediaObject));
            if (!acceptedMediaTypes.isEmpty() && !acceptedMediaTypes.contains(mappedMedia.Type)) {
                continue;
            }
            seenIds.insert(id);
            media.append(mappedMedia);
        }
    }
    return true;
}
