#ifndef HAIKENANIME_ACTIVESESSIONREGISTRY_H
#define HAIKENANIME_ACTIVESESSIONREGISTRY_H

#include <QList>

#include <functional>

#include "../../domain/recognition/EpisodeCandidate.h"
#include "../../domain/recognition/PlaybackSession.h"

struct ActiveSession final {
    PlaybackSession session;
    /// Candidate already associated with the local library (mediaId when it matched).
    EpisodeCandidate candidate;
    /// As reported by the player; a closed Plex session may linger as Paused until the server drops it.
    PlaybackState state = PlaybackState::Playing;
};

/// Sessions currently open in any plugin, for the "Assistindo agora" screen. Read-only view of
/// the pipeline: it never influences timing, progress or history.
class ActiveSessionRegistry final {
public:
    /// Adds the session, or replaces it when the same session moved to another episode.
    void start(const PlaybackSession &session, const EpisodeCandidate &candidate);
    void updateDetails(const PlaybackSessionId &sessionId, const EpisodeDetails &details);
    void updateState(const PlaybackSessionId &sessionId, PlaybackState state);
    void stop(const PlaybackSessionId &sessionId);
    void clear();

    /// Ordered by start time, then insertion.
    [[nodiscard]] QList<ActiveSession> sessions() const;
    /// Called after every effective change.
    void setChangedCallback(std::function<void()> callback);

private:
    void notify() const;

    QList<ActiveSession> sessions_;
    std::function<void()> changed_;
};

#endif // HAIKENANIME_ACTIVESESSIONREGISTRY_H
