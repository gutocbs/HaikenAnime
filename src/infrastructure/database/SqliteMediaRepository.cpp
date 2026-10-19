#include "SqliteMediaRepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDateTime>

#include "../../application/media/AlternativeNames.h"
#include "SqliteMediaMapper.h"
#include "SqlQueryStore.h"
#include "SqliteQueries.h"
#include "../logging/AsyncLogger.h"

#include <utility>

SqliteMediaRepository::SqliteMediaRepository(QSqlDatabase database)
    : database_(std::move(database)) {
}

void SqliteMediaRepository::setLogger(AsyncLogger *logger) { logger_ = logger; }

bool SqliteMediaRepository::readAll(QList<Media> &media, QString &error) {
    media.clear();
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }

    QString readQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadMedia, readQuery, error, logger_)) {
        return false;
    }

    QSqlQuery query(database_);
    if (!query.exec(readQuery)) {
        error = query.lastError().text();
        if (logger_) logger_->error(LogCategory::Database, error);
        return false;
    }

    while (query.next()) {
        media.append(SqliteMediaMapper::Map(query));
    }
    if (logger_) logger_->info(LogCategory::Database, QStringLiteral("Read %1 media records from SQLite.").arg(media.size()));
    return true;
}

bool SqliteMediaRepository::upsert(const QList<Media> &media, QString &error) {
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    QString upsertQuery;
    QString readNamesQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::UpsertMedia, upsertQuery, error, logger_)
        || !SqlQueryStore::loadSource(SqliteQueries::ReadMediaAlternativeNames, readNamesQuery, error, logger_)) {
        return false;
    }
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }

    QSqlQuery query(database_);
    if (!query.prepare(upsertQuery)) {
        error = query.lastError().text();
        database_.rollback();
        return false;
    }

    QSqlQuery readNames(database_);
    if (!readNames.prepare(readNamesQuery)) {
        error = readNames.lastError().text();
        database_.rollback();
        return false;
    }

    for (const auto &item : media) {
        // The sync must never erase names the user added: merge instead of replacing.
        QStringList existingNames;
        readNames.bindValue(QStringLiteral(":id"), item.Id);
        if (!readNames.exec()) {
            error = readNames.lastError().text();
            if (logger_) logger_->error(LogCategory::Database, error);
            database_.rollback();
            return false;
        }
        if (readNames.next()) existingNames = SqliteMediaMapper::ParseNames(readNames.value(0).toString());
        readNames.finish();

        SqliteMediaMapper::BindUpsert(query, item, MergeAlternativeNames(existingNames, item.AlternativeNames));

        if (!query.exec()) {
            error = query.lastError().text();
            if (logger_) logger_->error(LogCategory::Database, error);
            database_.rollback();
            return false;
        }
    }

    if (!database_.commit()) {
        error = database_.lastError().text();
        database_.rollback();
        return false;
    }
    if (logger_) logger_->info(LogCategory::Database, QStringLiteral("Upserted %1 media records into SQLite.").arg(media.size()));
    return true;
}

bool SqliteMediaRepository::reconcileAuthoritativeSnapshot(
    const QSet<int> &observedMediaIds, const QSet<int> &excludedMediaIds,
    int &removedCount, QString &error) {
    removedCount = 0;
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    QString readActiveMediaIdsQuery;
    QString markSourceRemovedQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadActiveMediaIds, readActiveMediaIdsQuery, error, logger_)
        || !SqlQueryStore::loadSource(SqliteQueries::MarkMediaSourceRemoved, markSourceRemovedQuery, error, logger_)) {
        return false;
    }
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }

    QSqlQuery active(database_);
    if (!active.exec(readActiveMediaIdsQuery)) {
        error = active.lastError().text();
        database_.rollback();
        return false;
    }
    QList<int> missingIds;
    while (active.next()) {
        const int mediaId = active.value(0).toInt();
        if (!observedMediaIds.contains(mediaId) && !excludedMediaIds.contains(mediaId)) missingIds.append(mediaId);
    }

    QSqlQuery mark(database_);
    if (!mark.prepare(markSourceRemovedQuery)) {
        error = mark.lastError().text();
        database_.rollback();
        return false;
    }
    const QString removedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    for (const int mediaId : missingIds) {
        mark.bindValue(QStringLiteral(":source_removed_at"), removedAt);
        mark.bindValue(QStringLiteral(":id"), mediaId);
        if (!mark.exec()) {
            error = mark.lastError().text();
            database_.rollback();
            removedCount = 0;
            return false;
        }
        removedCount += mark.numRowsAffected();
    }
    if (!database_.commit()) {
        error = database_.lastError().text();
        database_.rollback();
        removedCount = 0;
        return false;
    }
    return true;
}
