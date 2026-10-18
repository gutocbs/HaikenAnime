#ifndef HAIKENANIME_ILOCALEPISODEREADER_H
#define HAIKENANIME_ILOCALEPISODEREADER_H

#include <QString>

#include "LocalEpisode.h"

/** Reads the associated, available local files of a media for the Home's next-episode action. */
class ILocalEpisodeReader {
public:
    virtual ~ILocalEpisodeReader() = default;
    virtual bool readNextEpisode(int mediaId, int consumedEpisode,
                                 LocalEpisode &episode, QString &error) = 0;
    virtual bool readAvailableEpisodeCount(int mediaId, int &count, QString &error) = 0;
};

#endif
