#ifndef HAIKENANIME_IPLAYBACKPLUGIN_H
#define HAIKENANIME_IPLAYBACKPLUGIN_H

#include <QFlags>
#include <QString>

#include <functional>

#include "../../domain/recognition/EpisodeCandidate.h"
#include "../../domain/recognition/PlaybackSession.h"
#include "PluginSettingsSchema.h"

/// Optional actions the plugin settings screen offers (login in the browser, connection test, user list).
enum class PluginCapability {
    BrowserAuthentication = 0x01,
    ConnectionTest = 0x02,
    UserSelection = 0x04,
};
Q_DECLARE_FLAGS(PluginCapabilities, PluginCapability)
Q_DECLARE_OPERATORS_FOR_FLAGS(PluginCapabilities)

struct ConnectionTestResult final {
    bool succeeded = false;
    /// User-facing message; never contains credentials.
    QString safeMessage;
};

/// Receives normalized playback events. Plugins report what they observe; they never decide progress.
class IPlaybackObserver {
public:
    virtual ~IPlaybackObserver() = default;

    virtual void onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) = 0;
    virtual void onSessionUpdated(const PlaybackSessionId &sessionId, const PlaybackSnapshot &snapshot) = 0;
    virtual void onSessionStopped(const PlaybackSessionId &sessionId, StopReason reason) = 0;
    /// Episode data changed for an active session (e.g. a later Plex poll); optional for observers.
    virtual void onSessionDetailsChanged(const PlaybackSessionId &, const EpisodeDetails &) {}
};

/// Internal player integration (mpv, Plex) compiled with the application.
class IPlaybackPlugin {
public:
    virtual ~IPlaybackPlugin() = default;

    [[nodiscard]] virtual QString id() const = 0;
    [[nodiscard]] virtual QString displayName() const = 0;
    [[nodiscard]] virtual PluginCapabilities capabilities() const = 0;
    [[nodiscard]] virtual PluginSettingsSchema settingsSchema() const = 0;
    /// Starts reporting sessions to the observer, which must outlive the plugin or a call to stop().
    virtual void observe(IPlaybackObserver &observer) = 0;
    virtual void testConnection(std::function<void(const ConnectionTestResult &)> completion) = 0;
    /// Stops polling/IPC and releases resources. Safe to call more than once.
    virtual void stop() = 0;
};

#endif // HAIKENANIME_IPLAYBACKPLUGIN_H
