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
};

/// Polls /status/sessions of the selected server and reports episodes of the selected users only.
class PlexPlugin final : public IPlaybackPlugin {
public:
    static constexpr int DefaultPollSeconds = 15;
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

    void scheduleNextPoll(quint64 generation);
    void applySessions(const QList<PlexSession> &sessions);
    void stopAll(StopReason reason);
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
};

#endif // HAIKENANIME_PLEXPLUGIN_H
