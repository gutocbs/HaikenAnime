#include "SqliteMediaRepository.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include "SqliteMediaMapper.h"

#include <utility>

SqliteMediaRepository::SqliteMediaRepository(QSqlDatabase database, QString upsertQuery, QString readQuery)
    : database_(std::move(database)), upsertQuery_(std::move(upsertQuery)), readQuery_(std::move(readQuery)) {
}

QList<Media> SqliteMediaRepository::ReadAll(QString &error) {
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return {};
    }

    if (readQuery_.isEmpty()) {
        error = QStringLiteral("SQLite read query is empty.");
        return {};
    }

    QSqlQuery query(database_);
    if (!query.exec(readQuery_)) {
        error = query.lastError().text();
        return {};
    }

    QList<Media> media;
    while (query.next()) {
        media.append(SqliteMediaMapper::Map(query));
    }
    return media;
}

bool SqliteMediaRepository::Upsert(const QList<Media> &media, QString &error) {
    if (!database_.isOpen()) {
        error = QStringLiteral("SQLite database is not open.");
        return false;
    }
    if (upsertQuery_.isEmpty()) {
        error = QStringLiteral("SQLite upsert query is empty.");
        return false;
    }
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }

    QSqlQuery query(database_);
    query.prepare(upsertQuery_);

    for (const auto &item : media) {
        query.bindValue(QStringLiteral(":id"), item.Id);
        query.bindValue(QStringLiteral(":name"), item.Name);
        query.bindValue(QStringLiteral(":english_name"), item.EnglishName);
        query.bindValue(QStringLiteral(":original_name"), item.OriginalName);
        query.bindValue(QStringLiteral(":alternative_names"),
                        QString::fromUtf8(QJsonDocument::fromVariant(item.AlternativeNames).toJson()));
        query.bindValue(QStringLiteral(":total_chapters"), item.TotalChapters);
        query.bindValue(QStringLiteral(":average_score"), item.AverageScore);
        query.bindValue(QStringLiteral(":cover_url"), item.CoverUrl);
        query.bindValue(QStringLiteral(":synopsis"), item.Synopsis);
        query.bindValue(QStringLiteral(":type"), static_cast<int>(item.Type));
        query.bindValue(QStringLiteral(":status"), static_cast<int>(item.Status));

        if (!query.exec()) {
            error = query.lastError().text();
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
