#include "MpvPlugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

#include <utility>

struct MpvPlugin::ActiveSession {
    ActiveSession(PlaybackSession playbackSession, EpisodeCandidate episode, QString pipe)
        : session(std::move(playbackSession)), candidate(std::move(episode)), pipeName(std::move(pipe)) {}

    PlaybackSession session;
    EpisodeCandidate candidate;
    QString pipeName;
    std::unique_ptr<IMpvTransport> transport;
    std::unique_ptr<MpvIpcClient> client;
    bool reported = false;
    bool finished = false;
    PlaybackState state = PlaybackState::Playing;
    std::chrono::seconds position{0};
    QDateTime lastPositionReportAt;
};

namespace {
const auto PluginId = QStringLiteral("mpv");

bool SameFile(const QString &left, const QString &right) {
    return QDir::cleanPath(QDir::fromNativeSeparators(left))
               .compare(QDir::cleanPath(QDir::fromNativeSeparators(right)), Qt::CaseInsensitive) == 0;
}

StopReason StopReasonForEndFile(const QString &reason) {
    if (reason == QStringLiteral("eof")) return StopReason::Ended;
    if (reason == QStringLiteral("error")) return StopReason::Unavailable;
    if (reason == QStringLiteral("redirect")) return StopReason::Replaced;
    return StopReason::Closed;
}
}

MpvPlugin::MpvPlugin(MpvPluginDependencies dependencies) : dependencies_(std::move(dependencies)) {}

MpvPlugin::~MpvPlugin() {
    observer_ = nullptr;
    if (active_ && active_->client) active_->client->close();
}

QString MpvPlugin::id() const {
    return PluginId;
}

QString MpvPlugin::displayName() const {
    return QStringLiteral("mpv");
}

PluginCapabilities MpvPlugin::capabilities() const {
    return PluginCapability::DynamicSettings | PluginCapability::ConnectionTest | PluginCapability::ExplicitSessionEnd;
}

PluginSettingsSchema MpvPlugin::settingsSchema() const {
    return {.pluginId = PluginId,
            .displayName = displayName(),
            .fields = {{.key = QStringLiteral("enabled"),
                        .type = PluginSettingType::Boolean,
                        .label = QCoreApplication::translate("MpvPlugin", "Open episodes in mpv"),
                        .description = QCoreApplication::translate(
                            "MpvPlugin", "When disabled, episodes open in the default application without recognition."),
                        .defaultValue = false},
                       {.key = QStringLiteral("executablePath"),
                        .type = PluginSettingType::Text,
                        .label = QCoreApplication::translate("MpvPlugin", "mpv executable"),
                        .description = QCoreApplication::translate(
                            "MpvPlugin", "Program used to play episodes with automatic recognition."),
                        .required = true,
                        .defaultValue = QStringLiteral("mpv")}}};
}

void MpvPlugin::observe(IPlaybackObserver &observer) {
    observer_ = &observer;
}

void MpvPlugin::testConnection(std::function<void(const ConnectionTestResult &)> completion) {
    const bool available = dependencies_.launcher != nullptr && dependencies_.launcher->canStart(executable_);
    completion({.succeeded = available,
                .safeMessage = available ? QCoreApplication::translate("MpvPlugin", "mpv is available.")
                                         : QCoreApplication::translate("MpvPlugin", "mpv executable was not found.")});
}

void MpvPlugin::stop() {
    finishSession(StopReason::Closed);
}

void MpvPlugin::setExecutable(const QString &executable) {
    executable_ = executable.trimmed().isEmpty() ? QStringLiteral("mpv") : executable.trimmed();
}

std::optional<PlaybackSession> MpvPlugin::launchEpisode(const QString &filePath, const MpvLaunchOptions &options,
                                                        QString &error) {
    error.clear();
    if (!options.candidate.has_value()) {
        error = QStringLiteral("Automatic recognition needs an episode identified in the local library.");
        return std::nullopt;
    }
    if (dependencies_.launcher == nullptr || !dependencies_.transportFactory) {
        error = QStringLiteral("mpv integration is not configured.");
        return std::nullopt;
    }
    const auto pipeName = dependencies_.pipeNameFactory
        ? dependencies_.pipeNameFactory()
        : QStringLiteral("haikenanime-mpv-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    // Without --input-ipc-server mpv cannot be observed, so the argument is not optional.
    const QStringList arguments{QStringLiteral(R"(--input-ipc-server=\\.\pipe\)") + pipeName, QStringLiteral("--"),
                                filePath};
    if (!dependencies_.launcher->start(executable_, arguments, error)) {
        if (error.isEmpty()) error = QStringLiteral("mpv could not be started.");
        return std::nullopt;
    }

    retired_.clear();
    if (active_) {
        finishSession(StopReason::Replaced);
        retired_.push_back(std::move(active_));
    }

    PlaybackSession session{.id = PlaybackSessionId::create(PluginId, pipeName).value(),
                            .userId = QStringLiteral("local"),
                            .deviceId = QStringLiteral("local-mpv"),
                            .title = QFileInfo(filePath).fileName(),
                            .filePath = filePath,
                            .startedAt = now()};
    active_ = std::make_unique<ActiveSession>(session, *options.candidate, pipeName);
    auto *active = active_.get();
    active->transport = dependencies_.transportFactory();
    active->client = std::make_unique<MpvIpcClient>(
        *active->transport,
        MpvIpcCallbacks{
            .propertyChanged = [this, active](const QString &name, const QJsonValue &data) {
                if (active_.get() == active) onPropertyChanged(name, data);
            },
            .fileEnded = [this, active](const QString &reason) {
                if (active_.get() == active) finishSession(StopReasonForEndFile(reason));
            },
            .shutdown = [this, active] {
                if (active_.get() == active) finishSession(StopReason::Closed);
            },
            .disconnected = [this, active] {
                if (active_.get() == active) finishSession(StopReason::Closed);
            },
        });
    connectWithRetry(0);
    return session;
}

void MpvPlugin::connectWithRetry(const int attempt) {
    auto *active = active_.get();
    if (active == nullptr || active->finished) return;
    active->client->connect(active->pipeName, [this, active, attempt](const bool connected) {
        if (active_.get() != active || active->finished) return;
        if (connected) {
            onConnected();
            return;
        }
        if (attempt + 1 >= ConnectionAttempts) {
            // mpv never exposed IPC: the session is not trustworthy and is never reported.
            active->finished = true;
            return;
        }
        const auto retry = [this, active, attempt] {
            if (active_.get() == active) connectWithRetry(attempt + 1);
        };
        if (dependencies_.schedule) {
            dependencies_.schedule(ConnectionRetryDelayMs, retry);
        } else {
            retry();
        }
    });
}

void MpvPlugin::onConnected() {
    auto *active = active_.get();
    active->reported = true;
    active->client->observeProperty(PauseObserverId, QStringLiteral("pause"));
    active->client->observeProperty(TimePositionObserverId, QStringLiteral("time-pos"));
    active->client->observeProperty(PathObserverId, QStringLiteral("path"));
    if (observer_) observer_->onSessionStarted(active->session, active->candidate);
}

void MpvPlugin::onPropertyChanged(const QString &name, const QJsonValue &data) {
    auto *active = active_.get();
    if (active == nullptr || active->finished || !active->reported) return;
    const auto observedAt = now();
    if (name == QStringLiteral("path")) {
        const auto path = data.toString();
        if (!path.isEmpty() && !SameFile(path, active->session.filePath)) finishSession(StopReason::Replaced);
        return;
    }
    if (name == QStringLiteral("pause")) {
        if (!data.isBool()) return;
        active->state = data.toBool() ? PlaybackState::Paused : PlaybackState::Playing;
    } else if (name == QStringLiteral("time-pos")) {
        if (!data.isDouble()) return;
        active->position = std::chrono::seconds(static_cast<qint64>(data.toDouble()));
        // time-pos changes many times per second; one report per second is enough for the policy.
        if (active->lastPositionReportAt.isValid() && active->lastPositionReportAt.secsTo(observedAt) < 1) return;
        active->lastPositionReportAt = observedAt;
    } else {
        return;
    }
    if (observer_) {
        observer_->onSessionUpdated(active->session.id, {.position = active->position,
                                                         .state = active->state,
                                                         .available = true,
                                                         .observedAt = observedAt});
    }
}

void MpvPlugin::finishSession(const StopReason reason) {
    auto *active = active_.get();
    if (active == nullptr || active->finished) return;
    active->finished = true;
    if (active->reported && observer_) observer_->onSessionStopped(active->session.id, reason);
    if (active->client) active->client->close();
}

QDateTime MpvPlugin::now() const {
    return dependencies_.clock ? dependencies_.clock() : QDateTime::currentDateTimeUtc();
}
