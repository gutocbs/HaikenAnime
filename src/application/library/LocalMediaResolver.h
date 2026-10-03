#ifndef HAIKENANIME_LOCALMEDIARESOLVER_H
#define HAIKENANIME_LOCALMEDIARESOLVER_H

#include "LocalFileRecognitionTypes.h"
#include "../../domain/media/Media.h"

#include <QList>

struct LocalMediaMatch final {
    LocalRecognitionState state = LocalRecognitionState::Unrecognized;
    int mediaId = 0;
    QString diagnostic;
};

class LocalMediaResolver final {
public:
    LocalMediaMatch resolve(const LocalFileRecognition &recognition,
                            const QList<Media> &catalog) const;
};

#endif // HAIKENANIME_LOCALMEDIARESOLVER_H
