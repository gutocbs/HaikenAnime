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
    /// Effective media id: the correction's when the record was corrected, otherwise the candidate's.
    std::optional<int> mediaId;
    /// Only records whose effective episode is at most this value.
    std::optional<int> maxEpisode;
    /// Only records whose AniList synchronization is still pending or failed.
    bool awaitingSync = false;
    /// Cursor: only records with an id lower than this one.
    std::optional<qint64> beforeId;
    int limit = 50;
};

/// AniList progress change queued by a recognition, to be discarded together with its record.
struct QueuedProgress final {
    int mediaId = 0;
    int episode = 0;
};

class IRecognitionHistoryRepository {
public:
    virtual ~IRecognitionHistoryRepository() = default;

    /// Stores a new record and assigns its id. Fails when the deduplication key already exists.
    virtual bool insert(RecognitionRecord &record, QString &error) = 0;
    virtual bool update(const RecognitionRecord &record, QString &error) = 0;
    virtual bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &records, QString &error) = 0;
    /// Deletes the record and, when given, discards the matching AniList progress change that has not been
    /// sent yet — atomically. Local progress is never touched.
    virtual bool remove(qint64 recordId, const std::optional<QueuedProgress> &queuedProgress, QString &error) = 0;
};

#endif // HAIKENANIME_IRECOGNITIONHISTORYREPOSITORY_H
