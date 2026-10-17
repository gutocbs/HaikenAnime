#ifndef HAIKENANIME_MPVPLUGIN_H
#define HAIKENANIME_MPVPLUGIN_H

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "../../../application/recognition/IPlaybackPlugin.h"
#include "MpvIpcClient.h"

/// Starts external programs without waiting for them.
class IProcessLauncher {
public:
    virtual ~IProcessLauncher() = default;

    virtual bool start(const QString &program, const QStringList &arguments, QString &error) = 0;
    [[nodiscard]] virtual bool canStart(const QString &program) const = 0;
};

struct MpvLaunchOptions final {
    /// Episode known from the local library; without it the session is not reported.
    std::optional<EpisodeCandidate> candidate;
};

struct MpvPluginDependencies final {
    std::function<std::unique_ptr<IMpvTransport>()> transportFactory;
    IProcessLauncher *launcher = nullptr;
    /// Runs the callback after the delay (QTimer::singleShot in production, immediate in tests).
    std::function<void(int delayMs, std::function<void()>)> schedule;
    std::function<QDateTime()> clock;
    std::function<QString()> pipeNameFactory;
};

/// Recognizes episodes played in an mpv instance launched by HaikenAnime with JSON IPC.
/// External mpv instances without IPC are never reported.
class MpvPlugin final : public IPlaybackPlugin {
public:
    static constexpr int ConnectionAttempts = 20;
    static constexpr int ConnectionRetryDelayMs = 250;
    static constexpr int PauseObserverId = 1;
    static constexpr int TimePositionObserverId = 2;
    static constexpr int PathObserverId = 3;

    explicit MpvPlugin(MpvPluginDependencies dependencies);
    ~MpvPlugin() override;

    [[nodiscard]] QString id() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] PluginCapabilities capabilities() const override;
    [[nodiscard]] PluginSettingsSchema settingsSchema() const override;
    void observe(IPlaybackObserver &observer) override;
    void testConnection(std::function<void(const ConnectionTestResult &)> completion) override;
    void stop() override;

    void setExecutable(const QString &executable);
    /// Starts mpv with --input-ipc-server and returns the session it will report once IPC connects.
    std::optional<PlaybackSession> launchEpisode(const QString &filePath, const MpvLaunchOptions &options,
                                                 QString &error);

private:
    struct ActiveSession;

    void connectWithRetry(int attempt);
    void onConnected();
    void onPropertyChanged(const QString &name, const QJsonValue &data);
    void finishSession(StopReason reason);
    [[nodiscard]] QDateTime now() const;

    MpvPluginDependencies dependencies_;
    IPlaybackObserver *observer_ = nullptr;
    QString executable_ = QStringLiteral("mpv");
    std::unique_ptr<ActiveSession> active_;
    /// Replaced sessions stay alive until the next launch: their transport may still be
    /// delivering callbacks when they are replaced.
    std::vector<std::unique_ptr<ActiveSession>> retired_;
};

#endif // HAIKENANIME_MPVPLUGIN_H
