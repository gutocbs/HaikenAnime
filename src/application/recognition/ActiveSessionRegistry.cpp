#include "ActiveSessionRegistry.h"

#include <algorithm>
#include <utility>

namespace {
auto ById(const PlaybackSessionId &id) {
    return [&id](const ActiveSession &entry) { return entry.session.id == id; };
}
}

void ActiveSessionRegistry::start(const PlaybackSession &session, const EpisodeCandidate &candidate) {
    const auto found = std::find_if(sessions_.begin(), sessions_.end(), ById(session.id));
    if (found != sessions_.end()) {
        *found = {.session = session, .candidate = candidate};
    } else {
        // Stable insertion keeps start order; equal start times keep arrival order.
        const auto position = std::upper_bound(sessions_.begin(), sessions_.end(), session.startedAt,
                                               [](const QDateTime &startedAt, const ActiveSession &entry) {
                                                   return startedAt < entry.session.startedAt;
                                               });
        sessions_.insert(position, {.session = session, .candidate = candidate});
    }
    notify();
}

void ActiveSessionRegistry::updateDetails(const PlaybackSessionId &sessionId, const EpisodeDetails &details) {
    const auto found = std::find_if(sessions_.begin(), sessions_.end(), ById(sessionId));
    if (found == sessions_.end() || found->session.details == details) return;
    found->session.details = details;
    notify();
}

void ActiveSessionRegistry::updateState(const PlaybackSessionId &sessionId, const PlaybackState state) {
    const auto found = std::find_if(sessions_.begin(), sessions_.end(), ById(sessionId));
    if (found == sessions_.end() || found->state == state) return;
    found->state = state;
    notify();
}

void ActiveSessionRegistry::stop(const PlaybackSessionId &sessionId) {
    if (sessions_.removeIf(ById(sessionId)) > 0) notify();
}

void ActiveSessionRegistry::clear() {
    if (sessions_.isEmpty()) return;
    sessions_.clear();
    notify();
}

QList<ActiveSession> ActiveSessionRegistry::sessions() const {
    return sessions_;
}

void ActiveSessionRegistry::setChangedCallback(std::function<void()> callback) {
    changed_ = std::move(callback);
}

void ActiveSessionRegistry::notify() const {
    if (changed_) changed_();
}
