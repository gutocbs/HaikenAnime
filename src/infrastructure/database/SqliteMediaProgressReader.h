#ifndef HAIKENANIME_SQLITEMEDIAPROGRESSREADER_H
#define HAIKENANIME_SQLITEMEDIAPROGRESSREADER_H

#include <QSqlDatabase>

#include "../../application/recognition/ProgressDecisionService.h"
#include "SqliteQueries.h"

/// Reads the single media a recognition decides on, straight from SQLite: the decision must see the
/// current progress, never a cached library.
class SqliteMediaProgressReader final : public IMediaProgressReader {
public:
    explicit SqliteMediaProgressReader(
        QSqlDatabase database,
        QString readQuery = SqliteQueries::ReadMediaForRecognitionProgress);

    bool find(int mediaId, std::optional<Media> &media, QString &error) override;

private:
    QSqlDatabase database_;
    QString readQuery_;
};

#endif // HAIKENANIME_SQLITEMEDIAPROGRESSREADER_H
