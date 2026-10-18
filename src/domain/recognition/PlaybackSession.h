#ifndef HAIKENANIME_PLAYBACKSESSION_H
#define HAIKENANIME_PLAYBACKSESSION_H

#include <QDate>
#include <QDateTime>
#include <QString>

#include <chrono>
#include <optional>

/// Identity of one playback session inside one plugin (mpv pipe, Plex session key).
class PlaybackSessionId final {
public:
    /// Returns an id only when both the plugin id and the session key are non-blank.
    [[nodiscard]] static std::optional<PlaybackSessionId> create(const QString &pluginId,
                                                                 const QString &sessionKey);

    [[nodiscard]] const QString &pluginId() const { return m_pluginId; }
    [[nodiscard]] const QString &sessionKey() const { return m_sessionKey; }
    /// Stable textual key combining plugin and session, used for persistence and deduplication.
    [[nodiscard]] QString key() const;

    friend bool operator==(const PlaybackSessionId &, const PlaybackSessionId &) = default;

private:
    PlaybackSessionId(QString pluginId, QString sessionKey);

    QString m_pluginId;
    QString m_sessionKey;
};

enum class PlaybackState { Playing, Paused, Stopped };

enum class StopReason { Closed, Ended, Unavailable, Replaced };

/// Point-in-time observation reported by a plugin for an active session.
struct PlaybackSnapshot final {
    std::chrono::seconds position{0};
    PlaybackState state = PlaybackState::Playing;
    bool available = true;
    QDateTime observedAt;
};

/// Episode data a plugin already received with the playback (never fetched by HaikenAnime).
struct EpisodeDetails final {
    /// Raw name the plugin saw: file name (mpv) or series title (Plex).
    QString sourceTitle;
    QString title;
    QString summary;
    std::optional<int> durationSeconds;
    std::optional<QDate> airDate;

    friend bool operator==(const EpisodeDetails &, const EpisodeDetails &) = default;
};

/// Normalized playback session produced by a plugin. Plugins never decide progress.
struct PlaybackSession final {
    PlaybackSessionId id;
    QString userId;
    QString deviceId;
    /// Display names reported by the player (e.g. Plex account and "POCO X7 PRO"); ids stay the identity.
    QString userName;
    QString deviceName;
    QString title;
    QString filePath;
    QDateTime startedAt;
    EpisodeDetails details;
};

#endif // HAIKENANIME_PLAYBACKSESSION_H
