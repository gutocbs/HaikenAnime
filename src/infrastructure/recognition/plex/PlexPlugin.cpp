#include "PlexPlugin.h"

#include <QCoreApplication>

#include <utility>

namespace {
const auto PluginId = QStringLiteral("plex");

QString Translate(const char *text) {
    return QCoreApplication::translate("PlexPlugin", text);
}

/// Same user, device and episode: two session keys for it mean the older one was closed.
QString PlaybackKey(const PlexSession &session) {
    return QStringLiteral("%1|%2|%3|%4|%5")
        .arg(session.userId, session.playerId, session.showTitle)
        .arg(session.season.value_or(-1))
        .arg(session.episode.value_or(-1));
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
    return PluginCapability::BrowserAuthentication | PluginCapability::ConnectionTest
        | PluginCapability::UserSelection;
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
                        .maximum = MaximumPollSeconds}}};
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

void PlexPlugin::resume() {
    if (polling_ || observer_ == nullptr) return;
    polling_ = true;
    ++generation_;
    consecutiveFailures_ = 0;
    pollSessions();
}

void PlexPlugin::setServer(const PlexServer &server) {
    if (server_.has_value() && server_->clientIdentifier != server.clientIdentifier) stopAll(StopReason::Closed);
    server_ = server;
    wantedServerId_ = server.clientIdentifier;
}

void PlexPlugin::selectServer(const QString &serverId, std::function<void(bool, const QString &)> completion) {
    // Remembered even if this attempt fails, so later polls keep trying (e.g. network not up at boot).
    wantedServerId_ = serverId;
    QString token;
    QString error;
    if (dependencies_.authentication == nullptr || dependencies_.api == nullptr
        || !dependencies_.authentication->readToken(token, error)) {
        log(QStringLiteral("Plex server selection failed: server=%1 reason=not-connected").arg(serverId));
        completion(false, Translate("Connect your Plex account first."));
        return;
    }
    dependencies_.api->discoverServers(token, [this, serverId, completion = std::move(completion)](
                                                  const PlexApiError result, const QList<PlexServer> &servers) {
        if (result != PlexApiError::None) {
            log(QStringLiteral("Plex server selection failed: server=%1 reason=%2")
                    .arg(serverId, result == PlexApiError::Unauthorized ? QStringLiteral("unauthorized")
                                                                         : QStringLiteral("plex-tv-unavailable")));
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
        log(QStringLiteral("Plex server selection failed: server=%1 reason=not-found").arg(serverId));
        completion(false, Translate("The selected Plex server is no longer available."));
    });
}

void PlexPlugin::setSelectedUsers(const QSet<QString> &userIds) {
    selectedUsers_ = userIds;
}

void PlexPlugin::setPollInterval(const int seconds) {
    // Observation counts at most 60 s between two snapshots, so slower polling would undercount playback.
    pollSeconds_ = qBound(5, seconds, MaximumPollSeconds);
}

void PlexPlugin::pollSessions() {
    const auto generation = generation_;
    if (selectedUsers_.isEmpty() || dependencies_.api == nullptr || (!server_ && wantedServerId_.isEmpty())) {
        stopAll(StopReason::Closed);
        scheduleNextPoll(generation);
        return;
    }
    if (!server_.has_value()) {
        // The chosen server could not be reached before (boot without network, rejected server token).
        selectServer(wantedServerId_, [this, generation](const bool selected, const QString &message) {
            if (generation != generation_) return;
            if (selected) {
                log(QStringLiteral("Plex server selected: %1").arg(wantedServerId_));
                pollSessions();
                return;
            }
            lastError_ = message;
            scheduleNextPoll(generation);
        });
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
            consecutiveFailures_ = 0;
            applySessions(sessions);
            scheduleNextPoll(generation);
            return;
        case PlexApiError::Unauthorized:
            // The server token may have rotated: drop it and rediscover the server after a long pause.
            lastError_ = Translate("The Plex server rejected the login. Reconnect Plex.");
            stopAll(StopReason::Unavailable);
            server_.reset();
            consecutiveFailures_ = 0;
            scheduleNextPoll(generation, UnauthorizedRetrySeconds * 1000);
            return;
        case PlexApiError::Unavailable:
        case PlexApiError::InvalidResponse:
            lastError_ = Translate("The Plex server is unavailable.");
            // A single timeout must not reset the playback counted so far; the 60 s gap cap
            // already keeps the missing interval from counting.
            if (++consecutiveFailures_ >= FailuresBeforeSessionsEnd) stopAll(StopReason::Unavailable);
            scheduleNextPoll(generation);
            return;
        }
    });
}

void PlexPlugin::scheduleNextPoll(const quint64 generation, const int delayMs) {
    if (!polling_ || generation != generation_ || !dependencies_.schedule) return;
    dependencies_.schedule(delayMs >= 0 ? delayMs : pollSeconds_ * 1000, [this, generation] {
        if (polling_ && generation == generation_) pollSessions();
    });
}

void PlexPlugin::applySessions(const QList<PlexSession> &sessions) {
    const auto observedAt = now();
    // Plex keeps a closed session listed as "paused" for a while and opens a new one when the same
    // episode is reopened. Per user + device + episode, only the newest session key is real.
    const auto isNewer = [](const QString &left, const QString &right) {
        bool leftNumeric = false;
        bool rightNumeric = false;
        const auto leftNumber = left.toLongLong(&leftNumeric);
        const auto rightNumber = right.toLongLong(&rightNumeric);
        return leftNumeric && rightNumeric ? leftNumber > rightNumber : left > right;
    };
    QHash<QString, QString> newestByPlayback;
    for (const auto &plexSession : sessions) {
        if (plexSession.type != QStringLiteral("episode") || !plexSession.episode.has_value()) continue;
        const auto playback = PlaybackKey(plexSession);
        const auto current = newestByPlayback.value(playback);
        if (current.isEmpty() || isNewer(plexSession.sessionKey, current)) newestByPlayback.insert(playback, plexSession.sessionKey);
    }
    QSet<QString> replaced;
    QSet<QString> seen;
    for (const auto &plexSession : sessions) {
        if (plexSession.type != QStringLiteral("episode") || !selectedUsers_.contains(plexSession.userId)) continue;
        if (!plexSession.episode.has_value()) continue;
        const auto playback = PlaybackKey(plexSession);
        if (newestByPlayback.value(playback) != plexSession.sessionKey) {
            replaced.insert(PluginId + QLatin1Char(':') + server_->clientIdentifier + QLatin1Char(':') + plexSession.sessionKey);
            continue;
        }
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
        const bool wasReplaced = replaced.contains(entry.key());
        log(QStringLiteral("Plex session stopped: %1 reason=%2")
                .arg(entry.key(), wasReplaced ? QStringLiteral("replaced-by-newer-session")
                                              : QStringLiteral("no-longer-reported")));
        entry = tracked_.erase(entry);
        if (observer_) observer_->onSessionStopped(id, wasReplaced ? StopReason::Replaced : StopReason::Closed);
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
