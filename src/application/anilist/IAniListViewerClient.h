#ifndef HAIKENANIME_IANILISTVIEWERCLIENT_H
#define HAIKENANIME_IANILISTVIEWERCLIENT_H

#include <QString>

struct AniListViewer {
    qint64 id = 0;
    QString username;
};

class IAniListViewerClient {
public:
    virtual ~IAniListViewerClient() = default;
    virtual bool loadViewer(AniListViewer &viewer, QString &error) = 0;
};

#endif // HAIKENANIME_IANILISTVIEWERCLIENT_H
