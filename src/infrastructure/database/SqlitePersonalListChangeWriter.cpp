#include "SqlitePersonalListChangeWriter.h"

#include "SqlQueryStore.h"
#include "SqliteMediaMapper.h"
#include "SqlitePendingChangeCodec.h"
#include "SqliteQueries.h"
#include "../logging/AsyncLogger.h"

#include <QSqlError>
#include <QSqlQuery>

#include <utility>

SqlitePersonalListChangeWriter::SqlitePersonalListChangeWriter(QSqlDatabase database)
    : database_(std::move(database)) {
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
    if (!SqlQueryStore::loadSource(SqliteQueries::UpdatePersonalListMedia, updateQuery, error)
        || !SqlQueryStore::loadSource(SqliteQueries::UpsertMedia, upsertQuery, error)
        || !SqlQueryStore::loadSource(SqliteQueries::EnqueuePendingChange, enqueueQuery, error)) return false;
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
    SqliteMediaMapper::BindPersonalListUpdate(update, media);
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
        SqliteMediaMapper::BindUpsert(upsert, media, media.AlternativeNames);
        if (!upsert.exec()) {
            error = upsert.lastError().text();
            database_.rollback();
            return false;
        }
    }

    // Re-adding a media cancels any removal that has not reached AniList yet.
    QString supersedeQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::SupersedePendingDeletion, supersedeQuery, error)) {
        database_.rollback();
        return false;
    }
    QSqlQuery supersede(database_);
    if (!supersede.prepare(supersedeQuery)) {
        error = supersede.lastError().text();
        database_.rollback();
        return false;
    }
    supersede.bindValue(QStringLiteral(":media_id"), media.Id);
    supersede.bindValue(QStringLiteral(":local_updated_at"),
                        QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!supersede.exec()) {
        error = supersede.lastError().text();
        database_.rollback();
        return false;
    }
    const bool cancelledDeletion = supersede.numRowsAffected() > 0;

    QSqlQuery enqueue(database_);
    if (!enqueue.prepare(enqueueQuery)) {
        error = enqueue.lastError().text();
        database_.rollback();
        return false;
    }
    for (const auto &change : changes) {
        SqlitePendingChangeCodec::BindEnqueue(enqueue, change);
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
    if (cancelledDeletion && logger_) {
        logger_->info(LogCategory::Database,
                      QStringLiteral("Cancelled pending AniList list deletion for media %1 after re-add.")
                          .arg(media.Id));
    }
    return true;
}

void SqlitePersonalListChangeWriter::setLogger(AsyncLogger *logger) {
    logger_ = logger;
}

bool SqlitePersonalListChangeWriter::remove(const Media &media, const AniListPendingChange &deletion,
                                            QString &error) {
    error.clear();
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    QString markQuery;
    QString enqueueQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::MarkMediaPersonalListRemoved, markQuery, error)
        || !SqlQueryStore::loadSource(SqliteQueries::EnqueuePendingChange, enqueueQuery, error)) return false;
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }

    QSqlQuery mark(database_);
    if (!mark.prepare(markQuery)) {
        error = mark.lastError().text();
        database_.rollback();
        return false;
    }
    mark.bindValue(QStringLiteral(":id"), media.Id);
    mark.bindValue(QStringLiteral(":personal_list_removed_at"),
                   QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    if (!mark.exec()) {
        error = mark.lastError().text();
        database_.rollback();
        return false;
    }

    QSqlQuery enqueue(database_);
    if (!enqueue.prepare(enqueueQuery)) {
        error = enqueue.lastError().text();
        database_.rollback();
        return false;
    }
    SqlitePendingChangeCodec::BindEnqueue(enqueue, deletion);
    if (!enqueue.exec()) {
        error = enqueue.lastError().text();
        database_.rollback();
        return false;
    }
    if (!database_.commit()) {
        error = database_.lastError().text();
        database_.rollback();
        return false;
    }
    if (logger_) {
        logger_->info(LogCategory::Database,
                      QStringLiteral("Media %1 removed from the personal list.").arg(media.Id));
    }
    return true;
}
