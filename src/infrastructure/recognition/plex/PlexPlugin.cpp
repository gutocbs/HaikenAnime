#include "PlexPlugin.h"

#include <QCoreApplication>

#include <utility>

namespace {
const auto PluginId = QStringLiteral("plex");

QString Translate(const char *text) {
    return QCoreApplication::translate("PlexPlugin", text);
}

EpisodeDetails DetailsOf(const PlexSession &session) {
    return {.sourceTitle = session.showTitle,
            .title = session.episodeTitle,
            .summary = session.summary,
            .durationSeconds = session.durationMs > 0 ? std::optional<int>(static_cast<int>(session.durationMs / 1000))
                                                      : std::nullopt,
            .airDate = session.airDate};
}

PlaybackState StateOf(const QString &plexState) {
    // Buffering is not watching: only "playing" accumulates time.
    return plexState == QStringLiteral("playing") ? PlaybackState::Playing : PlaybackState::Paused;
}
}

PlexPlugin::PlexPlugin(PlexPluginDependencies dependencies) : dependencies_(std::move(dependencies)) {}

QString PlexPlugin::id() const {
    return PluginId;
}

QString PlexPlugin::displayName() const {
    return QStringLiteral("Plex");
}

PluginCapabilities PlexPlugin::capabilities() const {
    return PluginCapability::DynamicSettings | PluginCapability::BrowserAuthentication
        | PluginCapability::ConnectionTest | PluginCapability::UserSelection | PluginCapability::MultipleSessions;
}

PluginSettingsSchema PlexPlugin::settingsSchema() const {
    return {.pluginId = PluginId,
            .displayName = displayName(),
            .fields = {{.key = QStringLiteral("serverId"),
                        .type = PluginSettingType::Text,
                        .label = Translate("Server"),
                        .description = Translate("Plex Media Server whose sessions are observed.")},
                       {.key = QStringLiteral("selectedUsers"),
                        .type = PluginSettingType::Text,
                        .label = Translate("Observed users"),
                        .description = Translate("Plex accounts whose playback updates this profile.")},
                       {.key = QStringLiteral("pollSeconds"),
                        .type = PluginSettingType::Number,
                        .label = Translate("Polling interval (seconds)"),
                        .defaultValue = DefaultPollSeconds,
                        .minimum = 5,
                        .maximum = 300}}};
}

void PlexPlugin::observe(IPlaybackObserver &observer) {
    observer_ = &observer;
    polling_ = true;
    ++generation_;
    pollSessions();
}

void PlexPlugin::testConnection(std::function<void(const ConnectionTestResult &)> completion) {
    QString token;
    QString error;
    if (dependencies_.authentication == nullptr || dependencies_.api == nullptr
        || !dependencies_.authentication->readToken(token, error)) {
        completion({.succeeded = false, .safeMessage = Translate("Connect your Plex account first.")});
        return;
    }
    dependencies_.api->discoverServers(token, [completion = std::move(completion)](const PlexApiError result,
                                                                                    const QList<PlexServer> &servers) {
        if (result == PlexApiError::Unauthorized) {
            completion({.succeeded = false, .safeMessage = Translate("The Plex login is no longer valid.")});
        } else if (result != PlexApiError::None) {
            completion({.succeeded = false, .safeMessage = Translate("plex.tv could not be reached.")});
        } else if (servers.isEmpty()) {
            completion({.succeeded = false, .safeMessage = Translate("No Plex Media Server is available to this account.")});
        } else {
            completion({.succeeded = true, .safeMessage = Translate("Connected to Plex.")});
        }
    });
}

void PlexPlugin::stop() {
    polling_ = false;
    ++generation_;
    stopAll(StopReason::Closed);
}

void PlexPlugin::setServer(const PlexServer &server) {
    if (server_.has_value() && server_->clientIdentifier != server.clientIdentifier) stopAll(StopReason::Closed);
    server_ = server;
}

void PlexPlugin::selectServer(const QString &serverId, std::function<void(bool, const QString &)> completion) {
    QString token;
    QString error;
    if (dependencies_.authentication == nullptr || dependencies_.api == nullptr
        || !dependencies_.authentication->readToken(token, error)) {
        completion(false, Translate("Connect your Plex account first."));
        return;
    }
    dependencies_.api->discoverServers(token, [this, serverId, completion = std::move(completion)](
                                                  const PlexApiError result, const QList<PlexServer> &servers) {
        if (result != PlexApiError::None) {
            completion(false, Translate("Plex servers could not be listed."));
            return;
        }
        for (const auto &server : servers) {
            if (server.clientIdentifier == serverId) {
                setServer(server);
                completion(true, {});
                return;
            }
        }
        completion(false, Translate("The selected Plex server is no longer available."));
    });
}

void PlexPlugin::setSelectedUsers(const QSet<QString> &userIds) {
    selectedUsers_ = userIds;
}

void PlexPlugin::setPollInterval(const int seconds) {
    pollSeconds_ = qBound(5, seconds, 300);
}

void PlexPlugin::pollSessions() {
    const auto generation = generation_;
    if (!server_.has_value() || selectedUsers_.isEmpty() || dependencies_.api == nullptr) {
        stopAll(StopReason::Closed);
        scheduleNextPoll(generation);
        return;
    }
    dependencies_.api->listSessions(*server_, [this, generation](const PlexApiError result,
                                                                 const QList<PlexSession> &sessions) {
        if (generation != generation_) return;
        // One line per change of what the server reports, so a stuck session is visible in the log.
        QStringList reported;
        for (const auto &session : sessions) {
            reported.append(QStringLiteral("key=%1 type=%2 user=%3 state=%4 selected=%5")
                                .arg(session.sessionKey, session.type, session.userId, session.state,
                                     selectedUsers_.contains(session.userId) ? QStringLiteral("yes") : QStringLiteral("no")));
        }
        const auto summary = result == PlexApiError::None
            ? QStringLiteral("Plex poll: %1 session(s) [%2]").arg(sessions.size()).arg(reported.join(QStringLiteral("; ")))
            : QStringLiteral("Plex poll failed: %1").arg(result == PlexApiError::Unauthorized ? QStringLiteral("unauthorized")
                                                         : result == PlexApiError::Unavailable ? QStringLiteral("unavailable")
                                                                                               : QStringLiteral("invalid-response"));
        if (summary != lastPollSummary_) {
            lastPollSummary_ = summary;
            log(summary);
        }
        switch (result) {
        case PlexApiError::None:
            lastError_.clear();
            applySessions(sessions);
            scheduleNextPoll(generation);
            return;
        case PlexApiError::Unauthorized:
            // Polling again with a rejected token would only repeat the failure.
            lastError_ = Translate("The Plex server rejected the login. Reconnect Plex.");
            stopAll(StopReason::Unavailable);
            return;
        case PlexApiError::Unavailable:
        case PlexApiError::InvalidResponse:
            lastError_ = Translate("The Plex server is unavailable.");
            stopAll(StopReason::Unavailable);
            scheduleNextPoll(generation);
            return;
        }
    });
}

void PlexPlugin::scheduleNextPoll(const quint64 generation) {
    if (!polling_ || generation != generation_ || !dependencies_.schedule) return;
    dependencies_.schedule(pollSeconds_ * 1000, [this, generation] {
        if (polling_ && generation == generation_) pollSessions();
    });
}

void PlexPlugin::applySessions(const QList<PlexSession> &sessions) {
    const auto observedAt = now();
    QSet<QString> seen;
    for (const auto &plexSession : sessions) {
        if (plexSession.type != QStringLiteral("episode") || !selectedUsers_.contains(plexSession.userId)) continue;
        if (!plexSession.episode.has_value()) continue;
        const auto candidate = EpisodeCandidate::create(plexSession.showTitle, std::nullopt, plexSession.season,
                                                        *plexSession.episode, StructuredMetadataConfidence);
        // Session keys are only unique per server, so the server id is part of the identity.
        const auto sessionId = PlaybackSessionId::create(PluginId,
                                                         server_->clientIdentifier + QLatin1Char(':') + plexSession.sessionKey);
        if (!candidate.has_value() || !sessionId.has_value()) continue;
        const auto key = sessionId->key();
        seen.insert(key);

        const auto details = DetailsOf(plexSession);
        const auto tracked = tracked_.find(key);
        if (tracked == tracked_.end() || tracked->episodeKey != candidate->episodeKey()) {
            tracked_.insert(key, {.id = *sessionId, .episodeKey = candidate->episodeKey(), .details = details});
            log(QStringLiteral("Plex session started: %1 episode=%2 user=%3 device=%4")
                    .arg(key, candidate->episodeKey(), plexSession.userId, plexSession.playerId));
            if (observer_) {
                observer_->onSessionStarted({.id = *sessionId,
                                             .userId = plexSession.userId,
                                             .deviceId = plexSession.playerId,
                                             .userName = plexSession.userName,
                                             .deviceName = plexSession.playerTitle,
                                             .title = plexSession.episodeTitle,
                                             .startedAt = observedAt,
                                             .details = details},
                                            *candidate);
            }
        } else if (tracked->details != details) {
            // Same session and episode: only the displayed metadata changed between polls.
            tracked->details = details;
            if (observer_) observer_->onSessionDetailsChanged(*sessionId, details);
        }
        if (observer_) {
            observer_->onSessionUpdated(*sessionId, {.position = std::chrono::seconds(plexSession.viewOffsetMs / 1000),
                                                     .state = StateOf(plexSession.state),
                                                     .available = true,
                                                     .observedAt = observedAt});
        }
    }
    for (auto entry = tracked_.begin(); entry != tracked_.end();) {
        if (seen.contains(entry.key())) {
            ++entry;
            continue;
        }
        const auto id = entry->id;
        log(QStringLiteral("Plex session stopped: %1 reason=no-longer-reported").arg(entry.key()));
        entry = tracked_.erase(entry);
        if (observer_) observer_->onSessionStopped(id, StopReason::Closed);
    }
}

void PlexPlugin::stopAll(const StopReason reason) {
    const auto tracked = std::exchange(tracked_, {});
    for (const auto &session : tracked) {
        log(QStringLiteral("Plex session stopped: %1 reason=%2").arg(session.id.key(),
            reason == StopReason::Unavailable ? QStringLiteral("server-unavailable") : QStringLiteral("plugin-stopped")));
        if (observer_) observer_->onSessionStopped(session.id, reason);
    }
}

QDateTime PlexPlugin::now() const {
    return dependencies_.clock ? dependencies_.clock() : QDateTime::currentDateTimeUtc();
}

void PlexPlugin::log(const QString &message) const {
    if (dependencies_.log) dependencies_.log(message);
}
