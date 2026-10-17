#ifndef HAIKENANIME_RECOGNITIONCOMPOSITION_H
#define HAIKENANIME_RECOGNITIONCOMPOSITION_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QUrl>

#include <functional>
#include <memory>

#include "../application/media/IMediaReader.h"
#include "../application/media/PersonalListChangeService.h"
#include "../application/recognition/EpisodeRecognitionService.h"
#include "../application/recognition/IPluginSecretStore.h"
#include "../application/recognition/PersonalListProgressWriter.h"
#include "../application/recognition/PluginRegistry.h"
#include "../application/recognition/PluginSettingsService.h"
#include "../infrastructure/database/SqlitePluginSettingsRepository.h"
#include "../infrastructure/database/SqliteRecognitionHistoryRepository.h"
#include "../infrastructure/recognition/mpv/MpvPlugin.h"
#include "../infrastructure/recognition/plex/PlexPlugin.h"

struct RecognitionCompositionDependencies final {
    QSqlDatabase connection;
    IMediaReader *mediaReader = nullptr;
    const PersonalListChangeService *changes = nullptr;
    /// Defaults to Windows Credential Manager.
    std::unique_ptr<IPluginSecretStore> secretStore;
    /// Defaults to detached QProcess.
    std::unique_ptr<IProcessLauncher> processLauncher;
    /// Defaults to QDesktopServices::openUrl.
    std::function<bool(const QUrl &)> openUrl;
    /// Defaults to QTimer::singleShot bound to the context lifetime.
    std::function<void(int delayMs, std::function<void()>)> schedule;
    std::function<void(const QString &)> diagnostics;
    /// Called after automatic progress was saved locally (e.g. to refresh the Home screen).
    std::function<void(const Media &)> progressApplied;
};

/// Everything the episode recognition feature owns. Members are destroyed in reverse order, so the
/// pipeline (declared last) stops plugins before their dependencies go away.
struct RecognitionContext final {
    std::unique_ptr<QObject> timerOwner;
    std::unique_ptr<IPluginSecretStore> secrets;
    std::unique_ptr<SqlitePluginSettingsRepository> settingsRepository;
    std::unique_ptr<PluginSettingsService> settings;
    std::unique_ptr<SqliteRecognitionHistoryRepository> historyRepository;
    std::unique_ptr<PersonalListProgressWriter> progressWriter;
    std::unique_ptr<RecognitionHistoryService> history;
    std::unique_ptr<MediaReaderProgressReader> progressReader;
    std::unique_ptr<ProgressDecisionService> decisions;
    std::unique_ptr<CatalogEpisodeCandidateResolver> resolver;
    std::unique_ptr<IProcessLauncher> processLauncher;
    std::unique_ptr<QNetworkAccessManager> network;
    std::unique_ptr<IPlexHttp> plexHttp;
    std::unique_ptr<PlexApiClient> plexApi;
    std::unique_ptr<PlexAuthentication> plexAuthentication;
    PluginRegistry registry;
    MpvPlugin *mpv = nullptr;
    PlexPlugin *plex = nullptr;
    bool mpvEnabled = false;
    /// Non-fatal problems (unreadable settings, unavailable secret store); never contain secrets.
    QStringList warnings;
    std::unique_ptr<EpisodeRecognitionService> recognition;

    [[nodiscard]] bool isMpvEnabled() const { return mpvEnabled && mpv != nullptr; }
};

/// Builds and starts recognition. Never fails: problems become warnings so the app still opens.
[[nodiscard]] std::unique_ptr<RecognitionContext> createRecognitionContext(RecognitionCompositionDependencies dependencies);
/// Pushes saved plugin settings (threshold, mpv executable, Plex server/users/interval) into the runtime.
void applyRecognitionSettings(RecognitionContext &context);

#endif // HAIKENANIME_RECOGNITIONCOMPOSITION_H
