#ifndef HAIKENANIME_SQLITEMEDIAMAPPER_H
#define HAIKENANIME_SQLITEMEDIAMAPPER_H

#include <QSqlQuery>
#include <QStringList>

#include "../../domain/media/Media.h"

/** Converts between Media and the columns of the media table (read-media.sql, upsert-media.sql, …). */
class SqliteMediaMapper final {
public:
    static Media Map(const QSqlQuery &query);

    /** Binds every column of upsert-media.sql; alternativeNames is stored instead of media.AlternativeNames. */
    static void BindUpsert(QSqlQuery &query, const Media &media, const QStringList &alternativeNames);
    /** Binds the user-owned columns of update-personal-list-media.sql. */
    static void BindPersonalListUpdate(QSqlQuery &query, const Media &media);

    [[nodiscard]] static QString SerializeNames(const QStringList &names);
    [[nodiscard]] static QStringList ParseNames(const QString &json);
};

#endif // HAIKENANIME_SQLITEMEDIAMAPPER_H
