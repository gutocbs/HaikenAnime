#ifndef HAIKENANIME_LOCALEPISODE_H
#define HAIKENANIME_LOCALEPISODE_H

#include <QString>

struct LocalEpisode final {
    int mediaId = 0;
    int episode = 0;
    QString path;
};

#endif
