#ifndef HAIKENANIME_PROGRESSDECISIONSERVICE_H
#define HAIKENANIME_PROGRESSDECISIONSERVICE_H

#include <QString>

#include <optional>

#include "../../domain/media/Media.h"
#include "../../domain/recognition/EpisodeCandidate.h"
#include "../../domain/recognition/RecognitionPolicy.h"

/// Narrow read port over the local library used to decide automatic progress.
class IMediaProgressReader {
public:
    virtual ~IMediaProgressReader() = default;

    /// Sets media to the stored item (std::nullopt when unknown) and clears error on success.
    virtual bool find(int mediaId, std::optional<Media> &media, QString &error) = 0;
};

enum class ProgressDecisionKind { AlreadyWatched, MarkWatched, Uncertain, Ignored };

struct ProgressDecision final {
    ProgressDecisionKind kind = ProgressDecisionKind::Ignored;
    std::optional<Media> previous;
    /// Present only for MarkWatched: the media with the new progress and list status.
    std::optional<Media> updated;
    QString reason;
};

/// Decides what a completed observation does to local progress. Never regresses progress.
class ProgressDecisionService final {
public:
    ProgressDecisionService(IMediaProgressReader &reader, RecognitionPolicy policy);

    void setPolicy(const RecognitionPolicy &policy);
    [[nodiscard]] ProgressDecision evaluate(const EpisodeCandidate &candidate) const;

private:
    IMediaProgressReader &reader_;
    RecognitionPolicy policy_;
};

#endif // HAIKENANIME_PROGRESSDECISIONSERVICE_H
