#ifndef HAIKENANIME_EPISODERECOGNITIONSERVICE_H
#define HAIKENANIME_EPISODERECOGNITIONSERVICE_H

#include <QString>

#include <functional>
#include <memory>

#include "../media/IMediaReader.h"
#include "ActiveSessionRegistry.h"
#include "IPlaybackPlugin.h"
#include "PlaybackObservationService.h"
#include "PluginRegistry.h"
#include "ProgressDecisionService.h"
#include "RecognitionHistoryService.h"

/// Associates a plugin candidate with the local library (fills mediaId when the title matches).
class IEpisodeCandidateResolver {
public:
    virtual ~IEpisodeCandidateResolver() = default;

    [[nodiscard]] virtual EpisodeCandidate resolve(const EpisodeCandidate &candidate) = 0;
};

/// Resolves candidate titles against the local catalog with the library title matcher.
class CatalogEpisodeCandidateResolver final : public IEpisodeCandidateResolver {
public:
    explicit CatalogEpisodeCandidateResolver(IMediaReader &reader);

    [[nodiscard]] EpisodeCandidate resolve(const EpisodeCandidate &candidate) override;
    /// Drops the cached catalog; the next resolve reads the library again (call after the library changes).
    void invalidate();

private:
    IMediaReader &reader_;
    // Reading the whole library costs ~0.35 s; titles only need it again when the library changes.
    QList<Media> catalog_;
    bool catalogLoaded_ = false;
};

/// IMediaProgressReader over the existing media reader.
class MediaReaderProgressReader final : public IMediaProgressReader {
public:
    explicit MediaReaderProgressReader(IMediaReader &reader);

    bool find(int mediaId, std::optional<Media> &media, QString &error) override;

private:
    IMediaReader &reader_;
};

/// Pipeline: plugin events → library association → continuous-playback policy → progress decision
/// → history (+ AniList outbox through the progress writer). Owns no plugin.
class EpisodeRecognitionService final : public IPlaybackObserver {
public:
    EpisodeRecognitionService(PluginRegistry &registry, IEpisodeCandidateResolver &resolver,
                              ProgressDecisionService &decisions, RecognitionHistoryService &history,
                              RecognitionPolicy policy, std::function<void(const QString &)> diagnostics = {});
    ~EpisodeRecognitionService() override;

    void start();
    /// Stops every plugin (IPC, polling, timers) and ignores late events. Safe to call twice.
    void stop();
    void setPolicy(const RecognitionPolicy &policy);
    [[nodiscard]] bool isRunning() const { return running_; }
    [[nodiscard]] const ActiveSessionRegistry &activeSessions() const { return activeSessions_; }
    [[nodiscard]] ActiveSessionRegistry &activeSessions() { return activeSessions_; }

    void onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) override;
    void onSessionUpdated(const PlaybackSessionId &sessionId, const PlaybackSnapshot &snapshot) override;
    void onSessionStopped(const PlaybackSessionId &sessionId, StopReason reason) override;
    void onSessionDetailsChanged(const PlaybackSessionId &sessionId, const EpisodeDetails &details) override;

private:
    void complete(const PlaybackSession &session, const EpisodeCandidate &candidate);
    void incomplete(const PlaybackSession &session, const EpisodeCandidate &candidate);
    void report(const QString &message) const;

    PluginRegistry &registry_;
    IEpisodeCandidateResolver &resolver_;
    ProgressDecisionService &decisions_;
    RecognitionHistoryService &history_;
    std::function<void(const QString &)> diagnostics_;
    std::unique_ptr<PlaybackObservationService> observation_;
    ActiveSessionRegistry activeSessions_;
    bool running_ = false;
};

#endif // HAIKENANIME_EPISODERECOGNITIONSERVICE_H
