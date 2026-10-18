#ifndef HAIKENANIME_SQLITEMEDIAPROGRESSREADER_H
#define HAIKENANIME_SQLITEMEDIAPROGRESSREADER_H

#include <QSqlDatabase>

#include "../../application/recognition/ProgressDecisionService.h"

/// Reads the single media a recognition decides on, straight from SQLite: the decision must see the
/// current progress, never a cached library.
class SqliteMediaProgressReader final : public IMediaProgressReader {
public:
    explicit SqliteMediaProgressReader(QSqlDatabase database);

    bool find(int mediaId, std::optional<Media> &media, QString &error) override;

private:
    QSqlDatabase database_;
};

#endif // HAIKENANIME_SQLITEMEDIAPROGRESSREADER_H
