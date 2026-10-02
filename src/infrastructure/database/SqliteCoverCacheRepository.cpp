#include "SqliteCoverCacheRepository.h"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>

#include <utility>

SqliteCoverCacheRepository::SqliteCoverCacheRepository(QSqlDatabase database, QString readQuery,
                                                       QString upsertQuery, QString deleteQuery,
                                                       QString clearQuery)
    : database_(std::move(database)), readQuery_(std::move(readQuery)),
      upsertQuery_(std::move(upsertQuery)), deleteQuery_(std::move(deleteQuery)),
      clearQuery_(std::move(clearQuery)) {
}

bool SqliteCoverCacheRepository::ReadAll(QHash<int, CoverCacheEntry> &entries, QString &error) {
    entries.clear();
    error.clear();
    QSqlQuery query(database_);
    if (!query.exec(readQuery_)) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) {
        const auto quality = ParseCoverQuality(query.value(2).toString());
        const auto validatedAt = QDateTime::fromString(query.value(8).toString(), Qt::ISODate);
        if (!quality.has_value() || !validatedAt.isValid()) {
            entries.clear();
            error = QStringLiteral("Cover cache contains invalid quality or validation time.");
            return false;
        }
        CoverCacheEntry entry;
        entry.mediaId = query.value(0).toInt();
        entry.remoteUrl = query.value(1).toString();
        entry.quality = quality.value();
        entry.relativePath = query.value(3).toString();
        entry.mimeType = query.value(4).toString();
        entry.byteSize = query.value(5).toLongLong();
        entry.etag = query.value(6).toString();
        entry.lastModified = query.value(7).toString();
        entry.validatedAt = validatedAt.toUTC();
        entries.insert(entry.mediaId, entry);
    }
    return true;
}

bool SqliteCoverCacheRepository::Upsert(const CoverCacheEntry &entry, QString &error) {
    error.clear();
    if (entry.mediaId <= 0 || entry.remoteUrl.isEmpty() || entry.relativePath.isEmpty()
        || QDir::isAbsolutePath(entry.relativePath) || entry.mimeType.isEmpty()
        || entry.byteSize < 0 || !entry.validatedAt.isValid()) {
        error = QStringLiteral("Cover cache entry requires valid values and a relative path.");
        return false;
    }
    QSqlQuery query(database_);
    if (!query.prepare(upsertQuery_)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":media_id"), entry.mediaId);
    query.bindValue(QStringLiteral(":remote_url"), entry.remoteUrl);
    query.bindValue(QStringLiteral(":quality"), CoverQualityName(entry.quality));
    query.bindValue(QStringLiteral(":relative_path"), entry.relativePath);
    query.bindValue(QStringLiteral(":mime_type"), entry.mimeType);
    query.bindValue(QStringLiteral(":byte_size"), entry.byteSize);
    query.bindValue(QStringLiteral(":etag"), entry.etag.isNull() ? QStringLiteral("") : entry.etag);
    query.bindValue(QStringLiteral(":last_modified"),
                    entry.lastModified.isNull() ? QStringLiteral("") : entry.lastModified);
    query.bindValue(QStringLiteral(":validated_at"), entry.validatedAt.toUTC().toString(Qt::ISODate));
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

bool SqliteCoverCacheRepository::Remove(const int mediaId, QString &error) {
    error.clear();
    QSqlQuery query(database_);
    if (!query.prepare(deleteQuery_)) {
        error = query.lastError().text();
        return false;
    }
    query.bindValue(QStringLiteral(":media_id"), mediaId);
    if (!query.exec()) {
        error = query.lastError().text();
        return false;
    }
    return true;
}

bool SqliteCoverCacheRepository::Clear(QString &error) {
    error.clear();
    QSqlQuery query(database_);
    if (!query.exec(clearQuery_)) {
        error = query.lastError().text();
        return false;
    }
    return true;
}
