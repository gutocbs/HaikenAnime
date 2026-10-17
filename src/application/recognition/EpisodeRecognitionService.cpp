#include "EpisodeRecognitionService.h"

#include "../library/LocalMediaResolver.h"

#include <utility>

CatalogEpisodeCandidateResolver::CatalogEpisodeCandidateResolver(IMediaReader &reader) : reader_(reader) {}

EpisodeCandidate CatalogEpisodeCandidateResolver::resolve(const EpisodeCandidate &candidate) {
    if (candidate.mediaId().has_value() || candidate.title().isEmpty()) return candidate;
    QList<Media> catalog;
    QString error;
    if (!reader_.readAll(catalog, error)) return candidate;
    // Same title matcher the local library uses, so Plex titles and file names agree.
    const LocalFileRecognition recognition{.state = LocalRecognitionState::Recognized,
                                           .mediaKind = LocalMediaKind::Anime,
                                           .extractedTitle = candidate.title(),
                                           .season = candidate.season(),
                                           .episode = candidate.episode()};
    const auto match = LocalMediaResolver().resolve(recognition, catalog);
    const bool matched = match.mediaId > 0
        && (match.state == LocalRecognitionState::Recognized || match.state == LocalRecognitionState::Associated);
    if (!matched) return candidate;
    return EpisodeCandidate::create(candidate.title(), match.mediaId, candidate.season(), candidate.episode(),
                                    candidate.confidence())
        .value_or(candidate);
}

MediaReaderProgressReader::MediaReaderProgressReader(IMediaReader &reader) : reader_(reader) {}

bool MediaReaderProgressReader::find(const int mediaId, std::optional<Media> &media, QString &error) {
    media.reset();
    QList<Media> catalog;
    if (!reader_.readAll(catalog, error)) return false;
    for (const auto &item : catalog) {
        if (item.Id == mediaId) {
            media = item;
            break;
        }
    }
    return true;
}

EpisodeRecognitionService::EpisodeRecognitionService(PluginRegistry &registry, IEpisodeCandidateResolver &resolver,
                                                     ProgressDecisionService &decisions,
                                                     RecognitionHistoryService &history, const RecognitionPolicy policy,
                                                     std::function<void(const QString &)> diagnostics)
    : registry_(registry), resolver_(resolver), decisions_(decisions), history_(history),
      diagnostics_(std::move(diagnostics)) {
    observation_ = std::make_unique<PlaybackObservationService>(
        policy, ObservationCallbacks{
                    .completed = [this](const PlaybackSession &session, const EpisodeCandidate &candidate) {
                        complete(session, candidate);
                    },
                    .incomplete = [this](const PlaybackSession &session, const EpisodeCandidate &candidate,
                                         StopReason) { incomplete(session, candidate); }});
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
}

void EpisodeRecognitionService::setPolicy(const RecognitionPolicy &policy) {
    observation_->setPolicy(policy);
    decisions_.setPolicy(policy);
}

void EpisodeRecognitionService::onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    if (!running_) return;
    observation_->start(session, resolver_.resolve(candidate));
}

void EpisodeRecognitionService::onSessionUpdated(const PlaybackSessionId &sessionId, const PlaybackSnapshot &snapshot) {
    if (running_) observation_->update(sessionId, snapshot);
}

void EpisodeRecognitionService::onSessionStopped(const PlaybackSessionId &sessionId, const StopReason reason) {
    if (running_) observation_->stop(sessionId, reason);
}

void EpisodeRecognitionService::complete(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    const auto decision = decisions_.evaluate(candidate);
    QString error;
    if (!history_.record(session, candidate, decision, error)) {
        report(QStringLiteral("Recognition history could not be saved for %1: %2").arg(session.id.key(), error));
    }
}

void EpisodeRecognitionService::incomplete(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    QString error;
    if (!history_.recordIncomplete(session, candidate, error)) {
        report(QStringLiteral("Incomplete recognition could not be saved for %1: %2").arg(session.id.key(), error));
    }
}

void EpisodeRecognitionService::report(const QString &message) const {
    if (diagnostics_) diagnostics_(message);
}
