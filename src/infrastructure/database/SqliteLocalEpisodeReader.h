#ifndef HAIKENANIME_SQLITELOCALEPISODEREADER_H
#define HAIKENANIME_SQLITELOCALEPISODEREADER_H

#include <QSqlDatabase>

#include "../../application/library/ILocalEpisodeReader.h"

class SqliteLocalEpisodeReader final : public ILocalEpisodeReader {
public:
    explicit SqliteLocalEpisodeReader(QSqlDatabase database);
    bool readNextEpisode(int mediaId, int consumedEpisode,
                         LocalEpisode &episode, QString &error) override;
    bool readAvailableEpisodeCount(int mediaId, int &count, QString &error) override;
private:
    QSqlDatabase database_;
};

#endif
