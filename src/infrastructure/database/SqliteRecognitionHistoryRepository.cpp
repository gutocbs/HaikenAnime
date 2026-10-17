#include "SqliteRecognitionHistoryRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include "SqlQueryStore.h"

#include <utility>

namespace {
QVariant OptionalInt(const std::optional<int> value) {
    return value.has_value() ? QVariant(*value) : QVariant(QMetaType::fromType<int>());
}

std::optional<int> ReadOptionalInt(const QVariant &value) {
    if (value.isNull()) return std::nullopt;
    return value.toInt();
}

// A null QString binds as SQL NULL; required text columns store '' instead.
QVariant RequiredText(const QString &value) {
    return value.isNull() ? QVariant(QStringLiteral("")) : QVariant(value);
}

QString Timestamp(const QDateTime &value) {
    return value.toUTC().toString(Qt::ISODateWithMs);
}

void BindCandidates(QSqlQuery &query, const RecognitionRecord &record) {
    const auto &candidate = record.candidate();
    query.bindValue(QStringLiteral(":title"), RequiredText(candidate.title()));
    query.bindValue(QStringLiteral(":media_id"), OptionalInt(candidate.mediaId()));
    query.bindValue(QStringLiteral(":season"), OptionalInt(candidate.season()));
    query.bindValue(QStringLiteral(":episode"), candidate.episode());
    query.bindValue(QStringLiteral(":confidence"), candidate.confidence());

    const auto &correction = record.correction();
    const auto nullText = QVariant(QMetaType::fromType<QString>());
    const auto nullReal = QVariant(QMetaType::fromType<double>());
    query.bindValue(QStringLiteral(":correction_title"), correction ? QVariant(correction->title()) : nullText);
    query.bindValue(QStringLiteral(":correction_media_id"),
                    correction ? OptionalInt(correction->mediaId()) : OptionalInt(std::nullopt));
    query.bindValue(QStringLiteral(":correction_season"),
                    correction ? OptionalInt(correction->season()) : OptionalInt(std::nullopt));
    query.bindValue(QStringLiteral(":correction_episode"),
                    correction ? OptionalInt(correction->episode()) : OptionalInt(std::nullopt));
    query.bindValue(QStringLiteral(":correction_confidence"),
                    correction ? QVariant(correction->confidence()) : nullReal);
}

void BindState(QSqlQuery &query, const RecognitionRecord &record) {
    query.bindValue(QStringLiteral(":status"), ToString(record.status()));
    query.bindValue(QStringLiteral(":diagnostic"), RequiredText(record.diagnostic()));
    query.bindValue(QStringLiteral(":sync_state"), ToString(record.syncState()));
    query.bindValue(QStringLiteral(":sync_error"), RequiredText(record.syncError()));
    query.bindValue(QStringLiteral(":updated_at"), Timestamp(record.updatedAt()));
}

std::optional<RecognitionRecord> ReadRecord(const QSqlQuery &query) {
    const auto candidate = EpisodeCandidate::create(query.value(QStringLiteral("title")).toString(),
                                                    ReadOptionalInt(query.value(QStringLiteral("media_id"))),
                                                    ReadOptionalInt(query.value(QStringLiteral("season"))),
                                                    query.value(QStringLiteral("episode")).toInt(),
                                                    query.value(QStringLiteral("confidence")).toDouble());
    std::optional<EpisodeCandidate> correction;
    if (!query.value(QStringLiteral("correction_episode")).isNull()) {
        correction = EpisodeCandidate::create(query.value(QStringLiteral("correction_title")).toString(),
                                              ReadOptionalInt(query.value(QStringLiteral("correction_media_id"))),
                                              ReadOptionalInt(query.value(QStringLiteral("correction_season"))),
                                              query.value(QStringLiteral("correction_episode")).toInt(),
                                              query.value(QStringLiteral("correction_confidence")).toDouble());
        if (!correction.has_value()) return std::nullopt;
    }
    const auto status = RecognitionStatusFromString(query.value(QStringLiteral("status")).toString());
    const auto syncState = RecognitionSyncStateFromString(query.value(QStringLiteral("sync_state")).toString());
    if (!status.has_value() || !syncState.has_value()) return std::nullopt;

    return RecognitionRecord::restore({
        .id = query.value(QStringLiteral("id")).toLongLong(),
        .pluginId = query.value(QStringLiteral("plugin_id")).toString(),
        .sessionKey = query.value(QStringLiteral("session_key")).toString(),
        .userId = query.value(QStringLiteral("user_id")).toString(),
        .deviceId = query.value(QStringLiteral("device_id")).toString(),
        .candidate = candidate,
        .correction = correction,
        .status = *status,
        .diagnostic = query.value(QStringLiteral("diagnostic")).toString(),
        .syncState = *syncState,
        .syncError = query.value(QStringLiteral("sync_error")).toString(),
        .createdAt = QDateTime::fromString(query.value(QStringLiteral("created_at")).toString(), Qt::ISODateWithMs),
        .updatedAt = QDateTime::fromString(query.value(QStringLiteral("updated_at")).toString(), Qt::ISODateWithMs),
    });
}
}

SqliteRecognitionHistoryRepository::SqliteRecognitionHistoryRepository(QSqlDatabase database, QString readQuery,
                                                                       QString insertQuery, QString updateQuery)
    : database_(std::move(database)), readQuery_(std::move(readQuery)), insertQuery_(std::move(insertQuery)),
      updateQuery_(std::move(updateQuery)) {}

bool SqliteRecognitionHistoryRepository::insert(RecognitionRecord &record, QString &error) {
    error.clear();
    QString sql;
    if (!SqlQueryStore::loadSource(insertQuery_, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":plugin_id"), record.sessionId().pluginId());
    query.bindValue(QStringLiteral(":session_key"), record.sessionId().sessionKey());
    query.bindValue(QStringLiteral(":user_id"), RequiredText(record.userId()));
    query.bindValue(QStringLiteral(":device_id"), RequiredText(record.deviceId()));
    query.bindValue(QStringLiteral(":deduplication_key"), record.deduplicationKey());
    query.bindValue(QStringLiteral(":created_at"), Timestamp(record.createdAt()));
    BindCandidates(query, record);
    BindState(query, record);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    record.assignId(query.lastInsertId().toLongLong());
    return true;
}

bool SqliteRecognitionHistoryRepository::update(const RecognitionRecord &record, QString &error) {
    error.clear();
    QString sql;
    if (!SqlQueryStore::loadSource(updateQuery_, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":id"), record.id());
    BindCandidates(query, record);
    BindState(query, record);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    if (query.numRowsAffected() != 1) {
        error = QStringLiteral("Recognition record %1 does not exist.").arg(record.id());
        return false;
    }
    return true;
}

bool SqliteRecognitionHistoryRepository::find(const RecognitionHistoryQuery &criteria,
                                              QList<RecognitionRecord> &records, QString &error) {
    records.clear();
    error.clear();
    QString sql;
    if (!SqlQueryStore::loadSource(readQuery_, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    const auto nullId = QVariant(QMetaType::fromType<qint64>());
    const auto nullText = QVariant(QMetaType::fromType<QString>());
    query.bindValue(QStringLiteral(":record_id"), criteria.recordId ? QVariant(*criteria.recordId) : nullId);
    query.bindValue(QStringLiteral(":deduplication_key"),
                    criteria.deduplicationKey.isEmpty() ? nullText : QVariant(criteria.deduplicationKey));
    query.bindValue(QStringLiteral(":status"), criteria.status ? QVariant(ToString(*criteria.status)) : nullText);
    query.bindValue(QStringLiteral(":plugin_id"), criteria.pluginId.isEmpty() ? nullText : QVariant(criteria.pluginId));
    query.bindValue(QStringLiteral(":media_id"), OptionalInt(criteria.mediaId));
    query.bindValue(QStringLiteral(":max_episode"), OptionalInt(criteria.maxEpisode));
    query.bindValue(QStringLiteral(":awaiting_sync"), criteria.awaitingSync ? 1 : 0);
    query.bindValue(QStringLiteral(":before_id"), criteria.beforeId ? QVariant(*criteria.beforeId) : nullId);
    query.bindValue(QStringLiteral(":limit"), qMax(criteria.limit, 1));
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    QList<RecognitionRecord> loaded;
    while (query.next()) {
        auto record = ReadRecord(query);
        if (!record.has_value()) {
            error = QStringLiteral("Stored recognition record %1 is invalid.").arg(query.value(0).toLongLong());
            return false;
        }
        loaded.append(std::move(*record));
    }
    records = std::move(loaded);
    return true;
}
