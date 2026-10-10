#include "SqlitePersonalListChangeWriter.h"

#include "SqlQueryStore.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

namespace {
QString encodeValue(const AniListFieldValue &value) {
    QJsonObject object;
    if (std::holds_alternative<int>(value)) {
        object.insert(QStringLiteral("type"), QStringLiteral("int"));
        object.insert(QStringLiteral("value"), std::get<int>(value));
    } else {
        object.insert(QStringLiteral("type"), QStringLiteral("string"));
        object.insert(QStringLiteral("value"), std::get<QString>(value));
    }
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QString serializeLinks(const QList<MediaLink> &links) {
    QJsonArray array;
    for (const auto &link : links) {
        array.append(QJsonObject{{QStringLiteral("site"), link.Site},
                                 {QStringLiteral("url"), link.Url}});
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

void bindMediaUpsert(QSqlQuery &query, const Media &media) {
    query.bindValue(QStringLiteral(":id"), media.Id);
    query.bindValue(QStringLiteral(":name"), media.Name);
    query.bindValue(QStringLiteral(":english_name"), media.EnglishName);
    query.bindValue(QStringLiteral(":original_name"), media.OriginalName);
    query.bindValue(QStringLiteral(":alternative_names"),
                    QString::fromUtf8(QJsonDocument::fromVariant(media.AlternativeNames).toJson()));
    query.bindValue(QStringLiteral(":total_chapters"), media.TotalChapters);
    query.bindValue(QStringLiteral(":consumed_chapters"), media.ConsumedChapters);
    query.bindValue(QStringLiteral(":average_score"), media.AverageScore);
    query.bindValue(QStringLiteral(":personal_score"), media.PersonalScore);
    query.bindValue(QStringLiteral(":cover_url"), media.CoverUrl);
    query.bindValue(QStringLiteral(":cover_medium_url"), media.CoverMediumUrl);
    query.bindValue(QStringLiteral(":cover_large_url"), media.CoverLargeUrl);
    query.bindValue(QStringLiteral(":cover_extra_large_url"), media.CoverExtraLargeUrl);
    query.bindValue(QStringLiteral(":synopsis"), media.Synopsis);
    query.bindValue(QStringLiteral(":type"), static_cast<int>(media.Type));
    query.bindValue(QStringLiteral(":status"), static_cast<int>(media.Status));
    query.bindValue(QStringLiteral(":user_list_status"), static_cast<int>(media.ListStatus));
    query.bindValue(QStringLiteral(":local_path"), media.LocalPath.isNull() ? QStringLiteral("") : media.LocalPath);
    query.bindValue(QStringLiteral(":season"), media.Season);
    query.bindValue(QStringLiteral(":season_year"), media.SeasonYear.has_value() ? QVariant(*media.SeasonYear) : QVariant());
    query.bindValue(QStringLiteral(":next_airing_episode"), media.NextAiringEpisode.has_value() ? QVariant(*media.NextAiringEpisode) : QVariant());
    query.bindValue(QStringLiteral(":next_airing_at"), media.NextAiringAt.has_value() ? QVariant::fromValue(*media.NextAiringAt) : QVariant());
    query.bindValue(QStringLiteral(":anilist_url"), media.AniListUrl);
    query.bindValue(QStringLiteral(":external_links"), serializeLinks(media.ExternalLinks));
}

void bindPersonalListUpdate(QSqlQuery &query, const Media &media) {
    query.bindValue(QStringLiteral(":id"), media.Id);
    query.bindValue(QStringLiteral(":consumed_chapters"), media.ConsumedChapters);
    query.bindValue(QStringLiteral(":personal_score"), media.PersonalScore);
    query.bindValue(QStringLiteral(":user_list_status"), static_cast<int>(media.ListStatus));
    query.bindValue(QStringLiteral(":local_path"), media.LocalPath.isNull() ? QStringLiteral("") : media.LocalPath);
    query.bindValue(QStringLiteral(":alternative_names"),
                    QString::fromUtf8(QJsonDocument::fromVariant(media.AlternativeNames).toJson()));
}

void bindPendingChange(QSqlQuery &query, const AniListPendingChange &change) {
    query.bindValue(QStringLiteral(":media_id"), change.mediaId);
    query.bindValue(QStringLiteral(":field"), static_cast<int>(change.field));
    query.bindValue(QStringLiteral(":previous_value"), encodeValue(change.previousValue));
    query.bindValue(QStringLiteral(":new_value"), encodeValue(change.newValue));
    query.bindValue(QStringLiteral(":created_at"), change.createdAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":local_updated_at"), change.localUpdatedAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":remote_observed_at"), change.remoteObservedAt.isValid()
        ? change.remoteObservedAt.toString(Qt::ISODate) : QVariant());
    query.bindValue(QStringLiteral(":remote_version"),
                    change.remoteVersion.isNull() ? QStringLiteral("") : change.remoteVersion);
    query.bindValue(QStringLiteral(":attempts"), change.attempts);
    query.bindValue(QStringLiteral(":status"), static_cast<int>(change.status));
    query.bindValue(QStringLiteral(":last_error"),
                    change.lastError.isNull() ? QStringLiteral("") : change.lastError);
}
}

SqlitePersonalListChangeWriter::SqlitePersonalListChangeWriter(
    QSqlDatabase database, QString upsertMediaQuery, QString updatePersonalListQuery,
    QString enqueuePendingChangeQuery)
    : database_(std::move(database)), upsertMediaQuery_(std::move(upsertMediaQuery)),
      updatePersonalListQuery_(std::move(updatePersonalListQuery)),
      enqueuePendingChangeQuery_(std::move(enqueuePendingChangeQuery)) {
}

bool SqlitePersonalListChangeWriter::save(const Media &media, const QList<AniListPendingChange> &changes,
                                          QString &error) {
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    QString updateQuery;
    QString upsertQuery;
    QString enqueueQuery;
    if (!SqlQueryStore::loadSource(updatePersonalListQuery_, updateQuery, error)
        || !SqlQueryStore::loadSource(upsertMediaQuery_, upsertQuery, error)
        || !SqlQueryStore::loadSource(enqueuePendingChangeQuery_, enqueueQuery, error)) return false;
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }

    QSqlQuery update(database_);
    if (!update.prepare(updateQuery)) {
        error = update.lastError().text();
        database_.rollback();
        return false;
    }
    bindPersonalListUpdate(update, media);
    if (!update.exec()) {
        error = update.lastError().text();
        database_.rollback();
        return false;
    }
    if (update.numRowsAffected() == 0) {
        QSqlQuery upsert(database_);
        if (!upsert.prepare(upsertQuery)) {
            error = upsert.lastError().text();
            database_.rollback();
            return false;
        }
        bindMediaUpsert(upsert, media);
        if (!upsert.exec()) {
            error = upsert.lastError().text();
            database_.rollback();
            return false;
        }
    }

    QSqlQuery enqueue(database_);
    if (!enqueue.prepare(enqueueQuery)) {
        error = enqueue.lastError().text();
        database_.rollback();
        return false;
    }
    for (const auto &change : changes) {
        bindPendingChange(enqueue, change);
        if (!enqueue.exec()) {
            error = enqueue.lastError().text();
            database_.rollback();
            return false;
        }
    }
    if (!database_.commit()) {
        error = database_.lastError().text();
        database_.rollback();
        return false;
    }
    return true;
}
