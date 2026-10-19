#include "EpisodeRecognitionService.h"

#include "../library/LocalMediaResolver.h"

#include <utility>

CatalogEpisodeCandidateResolver::CatalogEpisodeCandidateResolver(IMediaReader &reader) : reader_(reader) {}

EpisodeCandidate CatalogEpisodeCandidateResolver::resolve(const EpisodeCandidate &candidate) {
    if (candidate.mediaId().has_value() || candidate.title().isEmpty()) return candidate;
    if (!catalogLoaded_) {
        QString error;
        // A failed read is not cached: the next session tries again.
        if (!reader_.readAll(catalog_, error)) return candidate;
        catalogLoaded_ = true;
    }
    // Same title matcher the local library uses, so Plex titles and file names agree.
    const LocalFileRecognition recognition{.state = LocalRecognitionState::Recognized,
                                           .extractedTitle = candidate.title(),
                                           .season = candidate.season(),
                                           .episode = candidate.episode()};
    // Only an association carries a media id.
    const auto match = LocalMediaResolver().resolve(recognition, catalog_);
    if (match.mediaId <= 0) return candidate;
    return EpisodeCandidate::create(candidate.title(), match.mediaId, candidate.season(), candidate.episode(),
                                    candidate.confidence())
        .value_or(candidate);
}

void CatalogEpisodeCandidateResolver::invalidate() {
    catalog_.clear();
    catalogLoaded_ = false;
}

EpisodeRecognitionService::EpisodeRecognitionService(PluginRegistry &registry, IEpisodeCandidateResolver &resolver,
                                                     ProgressDecisionService &decisions,
                                                     RecognitionHistoryService &history, const RecognitionPolicy policy,
                                                     std::function<void(const QString &)> diagnostics)
    : registry_(registry), resolver_(resolver), decisions_(decisions), history_(history),
      diagnostics_(std::move(diagnostics)) {
    observation_ = std::make_unique<PlaybackObservationService>(
        policy, ObservationCallbacks{
                    // A session closed before the threshold is not recorded: nothing was watched.
                    .completed = [this](const PlaybackSession &session, const EpisodeCandidate &candidate) {
                        complete(session, candidate);
                    }});
    decisions_.setPolicy(policy);
}

EpisodeRecognitionService::~EpisodeRecognitionService() {
    stop();
}

void EpisodeRecognitionService::start() {
    if (running_) return;
    running_ = true;
    for (auto *plugin : registry_.plugins()) plugin->observe(*this);
}

void EpisodeRecognitionService::stop() {
    if (!running_) return;
    // Plugins report their last stop events while running_ is still true, then late events are dropped.
    for (auto *plugin : registry_.plugins()) plugin->stop();
    running_ = false;
    activeSessions_.clear();
}

void EpisodeRecognitionService::setPolicy(const RecognitionPolicy &policy) {
    observation_->setPolicy(policy);
    decisions_.setPolicy(policy);
}

void EpisodeRecognitionService::onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    if (!running_) return;
    const auto resolved = resolver_.resolve(candidate);
    observation_->start(session, resolved);
    activeSessions_.start(session, resolved);
}

void EpisodeRecognitionService::onSessionUpdated(const PlaybackSessionId &sessionId, const PlaybackSnapshot &snapshot) {
    if (!running_) return;
    observation_->update(sessionId, snapshot);
    activeSessions_.updateState(sessionId, snapshot.state);
}

void EpisodeRecognitionService::onSessionStopped(const PlaybackSessionId &sessionId, const StopReason reason) {
    if (!running_) return;
    observation_->stop(sessionId, reason);
    activeSessions_.stop(sessionId);
}

void EpisodeRecognitionService::complete(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    const auto decision = decisions_.evaluate(candidate);
    QString error;
    if (!history_.record(session, candidate, decision, error)) {
        report(QStringLiteral("Recognition history could not be saved for %1: %2").arg(session.id.key(), error));
    }
}

void EpisodeRecognitionService::report(const QString &message) const {
    if (diagnostics_) diagnostics_(message);
}

void EpisodeRecognitionService::onSessionDetailsChanged(const PlaybackSessionId &sessionId,
                                                        const EpisodeDetails &details) {
    if (running_) activeSessions_.updateDetails(sessionId, details);
}
