#ifndef HAIKENANIME_SQLITEPENDINGCHANGECODEC_H
#define HAIKENANIME_SQLITEPENDINGCHANGECODEC_H

#include <QSqlQuery>
#include <QString>

#include "../../application/anilist/AniListPendingChange.h"

/** Storage format of anilist_pending_changes: values are JSON {"type": "int"|"string", "value": …}. */
namespace SqlitePendingChangeCodec {
[[nodiscard]] QString EncodeValue(const AniListFieldValue &value);
/** Fails for malformed JSON, unknown types and integers that do not fit an int. */
[[nodiscard]] bool DecodeValue(const QString &encoded, AniListFieldValue &value);
/** Binds every column of enqueue-pending-change.sql. */
void BindEnqueue(QSqlQuery &query, const AniListPendingChange &change);
}

#endif // HAIKENANIME_SQLITEPENDINGCHANGECODEC_H
