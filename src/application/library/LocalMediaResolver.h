#ifndef HAIKENANIME_LOCALMEDIARESOLVER_H
#define HAIKENANIME_LOCALMEDIARESOLVER_H

#include "LocalFileRecognitionTypes.h"
#include "LocalMediaCatalogIndex.h"
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
    // Preferred for batches: build the index once per catalog and reuse it for every file.
    LocalMediaMatch resolve(const LocalFileRecognition &recognition,
                            const LocalMediaCatalogIndex &index) const;
    // The media folder that contains the file wins over any title match; the episode still comes from
    // the recognition. An empty path skips the folder step.
    LocalMediaMatch resolve(const LocalFileRecognition &recognition,
                            const LocalMediaCatalogIndex &index,
                            const QString &absoluteFilePath) const;
};

#endif // HAIKENANIME_LOCALMEDIARESOLVER_H
