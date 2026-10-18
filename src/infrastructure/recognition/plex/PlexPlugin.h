#ifndef HAIKENANIME_PLEXPLUGIN_H
#define HAIKENANIME_PLEXPLUGIN_H

#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QString>

#include <functional>
#include <optional>

#include "../../../application/recognition/IPlaybackPlugin.h"
#include "PlexApiClient.h"
#include "PlexAuthentication.h"

struct PlexPluginDependencies final {
    PlexApiClient *api = nullptr;
    PlexAuthentication *authentication = nullptr;
    std::function<void(int delayMs, std::function<void()>)> schedule;
    std::function<QDateTime()> clock;
    /// Diagnostic lines (never contain tokens); routed to the structured log by the composition.
    std::function<void(const QString &)> log;
};

/// Polls /status/sessions of the selected server and reports episodes of the selected users only.
class PlexPlugin final : public IPlaybackPlugin {
public:
    static constexpr int DefaultPollSeconds = 15;
    static constexpr int MaximumPollSeconds = 60;
    /// A rejected token is retried rarely: the server token may have rotated, the account one may be revoked.
    static constexpr int UnauthorizedRetrySeconds = 300;
    /// Network hiccups shorter than this many consecutive failed polls keep sessions observed.
    static constexpr int FailuresBeforeSessionsEnd = 3;
    /// Plex metadata is structured, but the series still has to be matched to the local library.
    static constexpr double StructuredMetadataConfidence = 0.9;

    explicit PlexPlugin(PlexPluginDependencies dependencies);

    [[nodiscard]] QString id() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] PluginCapabilities capabilities() const override;
    [[nodiscard]] PluginSettingsSchema settingsSchema() const override;
    void observe(IPlaybackObserver &observer) override;
    void testConnection(std::function<void(const ConnectionTestResult &)> completion) override;
    void stop() override;
    /// Restarts polling after stop() (e.g. after reconnecting the account). No-op while polling.
    void resume();
    [[nodiscard]] bool isPolling() const { return polling_; }

    void setServer(const PlexServer &server);
    /// Rediscovers the account's servers and selects the one with the given id.
    void selectServer(const QString &serverId, std::function<void(bool selected, const QString &safeMessage)> completion);
    [[nodiscard]] std::optional<PlexServer> server() const { return server_; }
    [[nodiscard]] QSet<QString> selectedUsers() const { return selectedUsers_; }
    void setSelectedUsers(const QSet<QString> &userIds);
    void setPollInterval(int seconds);
    [[nodiscard]] QString lastError() const { return lastError_; }

    /// One poll of the server sessions; observe() schedules it repeatedly.
    void pollSessions();

private:
    struct TrackedSession {
        PlaybackSessionId id;
        QString episodeKey;
        EpisodeDetails details;
    };

    void scheduleNextPoll(quint64 generation, int delayMs = -1);
    void applySessions(const QList<PlexSession> &sessions);
    void stopAll(StopReason reason);
    void log(const QString &message) const;
    [[nodiscard]] QDateTime now() const;

    PlexPluginDependencies dependencies_;
    IPlaybackObserver *observer_ = nullptr;
    std::optional<PlexServer> server_;
    QSet<QString> selectedUsers_;
    int pollSeconds_ = DefaultPollSeconds;
    QHash<QString, TrackedSession> tracked_;
    quint64 generation_ = 0;
    bool polling_ = false;
    QString lastError_;
    QString lastPollSummary_;
    /// Server chosen by the user; kept while it cannot be reached so it is selected again later.
    QString wantedServerId_;
    int consecutiveFailures_ = 0;
};

#endif // HAIKENANIME_PLEXPLUGIN_H
