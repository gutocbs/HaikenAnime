#ifndef HAIKENANIME_LOCALEPISODEREADER_H
#define HAIKENANIME_LOCALEPISODEREADER_H

#include <QSqlDatabase>
#include <QString>

struct LocalEpisode final {
    int mediaId = 0;
    int episode = 0;
    QString path;
};

class ILocalEpisodeReader {
public:
    virtual ~ILocalEpisodeReader() = default;
    virtual bool readNextEpisode(int mediaId, int consumedEpisode,
                                 LocalEpisode &episode, QString &error) = 0;
    virtual bool readAvailableEpisodeCount(int mediaId, int &count, QString &error) = 0;
};

class LocalEpisodeReader final : public ILocalEpisodeReader {
public:
    LocalEpisodeReader(QSqlDatabase database, QString query);
    bool readNextEpisode(int mediaId, int consumedEpisode,
                         LocalEpisode &episode, QString &error) override;
    bool readAvailableEpisodeCount(int mediaId, int &count, QString &error) override;
private:
    QSqlDatabase database_;
    QString query_;
};

#endif
