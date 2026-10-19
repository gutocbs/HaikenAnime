#ifndef HAIKENANIME_RECOGNITIONHISTORYSERVICE_H
#define HAIKENANIME_RECOGNITIONHISTORYSERVICE_H

#include <QDateTime>
#include <QList>
#include <QString>

#include <functional>
#include <optional>

#include "IRecognitionHistoryRepository.h"
#include "IRecognitionProgressWriter.h"
#include "ProgressDecisionService.h"

/// Player shown for history entries made by manual edits (Home and seasonal catalog).
inline const QString ManualPluginId = QStringLiteral("manual");

struct RecognitionHistoryFilter final {
    std::optional<RecognitionStatus> status;
    QString pluginId;
};

struct RecognitionPage final {
    int limit = 50;
    std::optional<qint64> beforeId;
};

/// Persists recognition outcomes, applies MarkWatched decisions through the progress writer
/// exactly once per session/episode, and keeps records correctable.
class RecognitionHistoryService final {
public:
    RecognitionHistoryService(IRecognitionHistoryRepository &repository, IRecognitionProgressWriter &progress,
                              std::function<QDateTime()> clock = {});

    /// Records a completed observation. A repeated session/episode with a final outcome returns the
    /// existing record and never applies progress again.
    std::optional<RecognitionRecord> record(const PlaybackSession &session, const EpisodeCandidate &candidate,
                                            const ProgressDecision &decision, QString &error);

    /// Records a manual progress edit by ManualPluginId, awaiting its AniList sync: a raise as MarkedWatched,
    /// a decrease as ProgressReduced. Edits that keep the progress record nothing (std::nullopt, empty error).
    std::optional<RecognitionRecord> recordManualProgress(const Media &previous, const Media &updated,
                                                          QString &error);
    /// Records a manual removal from the personal list, awaiting its AniList sync.
    std::optional<RecognitionRecord> recordManualRemoval(const Media &removed, QString &error);
    /// Removed records of mediaId whose AniList sync is pending or failed.
    bool findAwaitingRemovalSync(int mediaId, QList<RecognitionRecord> &records, QString &error);

    bool list(const RecognitionHistoryFilter &filter, const RecognitionPage &page,
              QList<RecognitionRecord> &records, QString &error);
    /// Replaces the recognized episode; a MarkWatched decision for the correction is applied.
    bool correct(qint64 recordId, const EpisodeCandidate &correction, const ProgressDecision &decision,
                 QString &error);
    bool ignore(qint64 recordId, QString &error);
    /// Deletes the record from the local history; a progress change it queued for AniList and not yet
    /// sent is discarded with it. Local progress stays as it is.
    bool remove(qint64 recordId, QString &error);
    /// At startup: records left half-written by a crash become Uncertain ("interrupted") instead of
    /// staying Pending forever or claiming a sync that was never queued.
    bool recoverInterrupted(QString &error);
    /// Durable, visible AniList failure. Local progress stays as it is.
    bool markSyncFailure(qint64 recordId, const QString &safeError, QString &error);
    bool markSynced(qint64 recordId, QString &error);
    /// Records whose AniList sync is pending or failed for mediaId and that a sent progress settles:
    /// MarkedWatched/Corrected up to an episode, and every ProgressReduced (compaction may change the value).
    bool findAwaitingSync(int mediaId, int upToEpisode, QList<RecognitionRecord> &records, QString &error);

private:
    [[nodiscard]] QDateTime now() const;
    std::optional<RecognitionRecord> load(qint64 recordId, QString &error);
    bool applyProgress(RecognitionRecord &record, const ProgressDecision &decision);
    std::optional<RecognitionRecord> recordManual(const Media &media, int episode, RecognitionStatus status,
                                                  QString &error);

    IRecognitionHistoryRepository &repository_;
    IRecognitionProgressWriter &progress_;
    std::function<QDateTime()> clock_;
};

#endif // HAIKENANIME_RECOGNITIONHISTORYSERVICE_H
