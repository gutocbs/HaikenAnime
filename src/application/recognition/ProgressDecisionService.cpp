#include "ProgressDecisionService.h"

ProgressDecisionService::ProgressDecisionService(IMediaProgressReader &reader, RecognitionPolicy policy)
    : reader_(reader), policy_(policy) {}

void ProgressDecisionService::setPolicy(const RecognitionPolicy &policy) {
    policy_ = policy;
}

ProgressDecision ProgressDecisionService::evaluate(const EpisodeCandidate &candidate) const {
    if (!policy_.isConfident(candidate)) {
        return {.kind = ProgressDecisionKind::Uncertain, .reason = QStringLiteral("low-confidence")};
    }
    if (!candidate.mediaId().has_value()) {
        return {.kind = ProgressDecisionKind::Uncertain, .reason = QStringLiteral("media-not-identified")};
    }

    std::optional<Media> media;
    QString error;
    if (!reader_.find(*candidate.mediaId(), media, error)) {
        return {.kind = ProgressDecisionKind::Ignored, .reason = QStringLiteral("media-lookup-failed: %1").arg(error)};
    }
    if (!media.has_value()) {
        return {.kind = ProgressDecisionKind::Ignored, .reason = QStringLiteral("media-not-in-library")};
    }

    const int episode = candidate.episode();
    if (episode <= media->ConsumedChapters) {
        return {.kind = ProgressDecisionKind::AlreadyWatched, .previous = media};
    }
    if (media->TotalChapters > 0 && episode > media->TotalChapters) {
        return {.kind = ProgressDecisionKind::Uncertain, .previous = media,
                .reason = QStringLiteral("episode-beyond-total")};
    }

    Media updated = *media;
    updated.ConsumedChapters = episode;
    updated.ListStatus = media->TotalChapters > 0 && episode == media->TotalChapters ? UserListStatus::Completed
                                                                                     : UserListStatus::Current;
    return {.kind = ProgressDecisionKind::MarkWatched, .previous = media, .updated = updated};
}
