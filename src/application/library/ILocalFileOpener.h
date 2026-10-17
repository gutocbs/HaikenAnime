#ifndef HAIKENANIME_ILOCALFILEOPENER_H
#define HAIKENANIME_ILOCALFILEOPENER_H

#include <QString>

#include "LocalEpisode.h"

class ILocalFileOpener {
public:
    virtual ~ILocalFileOpener() = default;
    virtual bool open(const QString &path, QString &error) = 0;
    /// Opens a known library episode; players that recognize playback (mpv) need its identity.
    virtual bool openEpisode(const LocalEpisode &episode, QString &error) { return open(episode.path, error); }
};

#endif
