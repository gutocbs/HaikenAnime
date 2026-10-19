#ifndef HAIKENANIME_ILOCALFILERECOGNITIONREPOSITORY_H
#define HAIKENANIME_ILOCALFILERECOGNITIONREPOSITORY_H

#include <QList>
#include "../../domain/library/LocalFileRecord.h"
#include "../../domain/media/Media.h"

/** Reads local files to recognize and saves their associations (used by the recognition coordinator). */
class ILocalFileRecognitionRepository {
public:
    virtual ~ILocalFileRecognitionRepository() = default;

    // Available files of the root that are unprocessed, unrecognized or ambiguous.
    virtual bool readPendingRecognition(const QString &rootPath, QList<LocalFileRecognitionRecord> &records,
                                        QString &error) = 0;
    // Every available file whose absolute path is inside the folder (already associated ones included).
    virtual bool readFilesUnderFolder(const QString &folderPath, QList<LocalFileRecognitionRecord> &records,
                                      QString &error) = 0;
    virtual bool readRecognitionCatalog(QList<Media> &media, QString &error) = 0;
    virtual bool saveRecognitionBatch(const QList<LocalFileRecognitionRecord> &records, QString &error) = 0;
};

#endif
