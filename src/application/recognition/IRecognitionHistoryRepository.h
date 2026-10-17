#ifndef HAIKENANIME_IRECOGNITIONHISTORYREPOSITORY_H
#define HAIKENANIME_IRECOGNITIONHISTORYREPOSITORY_H

#include <QList>
#include <QString>

#include <optional>

#include "../../domain/recognition/RecognitionRecord.h"

/// Every criterion is optional; results are ordered newest first (descending id).
struct RecognitionHistoryQuery final {
    std::optional<qint64> recordId;
    QString deduplicationKey;
    std::optional<RecognitionStatus> status;
    QString pluginId;
    /// Cursor: only records with an id lower than this one.
    std::optional<qint64> beforeId;
    int limit = 50;
};

class IRecognitionHistoryRepository {
public:
    virtual ~IRecognitionHistoryRepository() = default;

    /// Stores a new record and assigns its id. Fails when the deduplication key already exists.
    virtual bool insert(RecognitionRecord &record, QString &error) = 0;
    virtual bool update(const RecognitionRecord &record, QString &error) = 0;
    virtual bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &records, QString &error) = 0;
};

#endif // HAIKENANIME_IRECOGNITIONHISTORYREPOSITORY_H
