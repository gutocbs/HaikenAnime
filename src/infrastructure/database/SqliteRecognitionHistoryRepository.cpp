#include "SqliteRecognitionHistoryRepository.h"

#include <QSqlError>
#include <QSqlQuery>

#include "SqlQueryStore.h"
#include "../../application/anilist/AniListPendingChangeStatus.h"
#include "../../domain/anilist/AniListField.h"

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
                                                                       QString insertQuery, QString updateQuery,
                                                                       QString deleteQuery, QString discardProgressQuery,
                                                                       QString queuedProgressQuery)
    : database_(std::move(database)), readQuery_(std::move(readQuery)), insertQuery_(std::move(insertQuery)),
      updateQuery_(std::move(updateQuery)), deleteQuery_(std::move(deleteQuery)),
      discardProgressQuery_(std::move(discardProgressQuery)),
      queuedProgressQuery_(std::move(queuedProgressQuery)) {}

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
    const int limit = qMax(criteria.limit, 1);
    auto beforeId = criteria.beforeId;
    QList<RecognitionRecord> loaded;
    // A corrupted row is skipped instead of failing the whole page; reading resumes after the last row seen
    // so the page still comes back full and pagination does not stop early.
    while (true) {
        query.bindValue(QStringLiteral(":record_id"), criteria.recordId ? QVariant(*criteria.recordId) : nullId);
        query.bindValue(QStringLiteral(":deduplication_key"),
                        criteria.deduplicationKey.isEmpty() ? nullText : QVariant(criteria.deduplicationKey));
        query.bindValue(QStringLiteral(":status"), criteria.status ? QVariant(ToString(*criteria.status)) : nullText);
        query.bindValue(QStringLiteral(":plugin_id"),
                        criteria.pluginId.isEmpty() ? nullText : QVariant(criteria.pluginId));
        query.bindValue(QStringLiteral(":media_id"), OptionalInt(criteria.mediaId));
        query.bindValue(QStringLiteral(":max_episode"), OptionalInt(criteria.maxEpisode));
        query.bindValue(QStringLiteral(":awaiting_sync"), criteria.awaitingSync ? 1 : 0);
        query.bindValue(QStringLiteral(":before_id"), beforeId ? QVariant(*beforeId) : nullId);
        query.bindValue(QStringLiteral(":limit"), limit - loaded.size());
        if (!query.exec()) {
            error = query.lastError().text();
            return false;
        }
        int rows = 0;
        bool skipped = false;
        while (query.next()) {
            ++rows;
            beforeId = query.value(QStringLiteral("id")).toLongLong();
            auto record = ReadRecord(query);
            if (record.has_value()) {
                loaded.append(std::move(*record));
            } else {
                skipped = true;
            }
        }
        if (!skipped || rows == 0 || loaded.size() >= limit) break;
    }
    records = std::move(loaded);
    return true;
}

bool SqliteRecognitionHistoryRepository::remove(const qint64 recordId, const std::optional<QueuedProgress> &queuedProgress,
                                                QString &error) {
    error.clear();
    QString deleteSql;
    QString discardSql;
    if (!SqlQueryStore::loadSource(deleteQuery_, deleteSql, error)
        || (queuedProgress && !SqlQueryStore::loadSource(discardProgressQuery_, discardSql, error))) {
        return false;
    }
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }
    const auto fail = [this, &error](const QString &message) {
        error = message;
        database_.rollback();
        return false;
    };

    // The record and the change it queued disappear together, or neither does.
    if (queuedProgress) {
        QSqlQuery discard(database_);
        if (!discard.prepare(discardSql)) return fail(discard.lastError().text());
        discard.bindValue(QStringLiteral(":superseded_status"), static_cast<int>(AniListPendingChangeStatus::Superseded));
        discard.bindValue(QStringLiteral(":media_id"), queuedProgress->mediaId);
        discard.bindValue(QStringLiteral(":progress_field"), static_cast<int>(AniListField::Progress));
        discard.bindValue(QStringLiteral(":pending_status"), static_cast<int>(AniListPendingChangeStatus::Pending));
        discard.bindValue(QStringLiteral(":failed_status"), static_cast<int>(AniListPendingChangeStatus::Failed));
        discard.bindValue(QStringLiteral(":confirmation_status"),
                          static_cast<int>(AniListPendingChangeStatus::RequiresConfirmation));
        discard.bindValue(QStringLiteral(":episode"), queuedProgress->episode);
        if (!discard.exec()) return fail(discard.lastError().text());
    }

    QSqlQuery remove(database_);
    if (!remove.prepare(deleteSql)) return fail(remove.lastError().text());
    remove.bindValue(QStringLiteral(":id"), recordId);
    if (!remove.exec()) return fail(remove.lastError().text());
    if (remove.numRowsAffected() != 1) {
        return fail(QStringLiteral("Recognition record %1 does not exist.").arg(recordId));
    }
    if (!database_.commit()) return fail(database_.lastError().text());
    return true;
}

bool SqliteRecognitionHistoryRepository::hasQueuedProgress(const QueuedProgress &progress, bool &found,
                                                           QString &error) {
    error.clear();
    found = false;
    QString sql;
    if (!SqlQueryStore::loadSource(queuedProgressQuery_, sql, error)) return false;
    QSqlQuery query(database_);
    if (!query.prepare(sql)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":media_id"), progress.mediaId);
    query.bindValue(QStringLiteral(":progress_field"), static_cast<int>(AniListField::Progress));
    query.bindValue(QStringLiteral(":superseded_status"), static_cast<int>(AniListPendingChangeStatus::Superseded));
    query.bindValue(QStringLiteral(":episode"), progress.episode);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    found = query.next();
    return true;
}
