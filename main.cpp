#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

#include <memory>
#include <utility>
#include <vector>

#include "src/app/ApplicationComposition.h"
#include "src/app/TranslationLoader.h"
#include "src/presentation/home/HomeScreenController.h"
#include "src/presentation/seasonal/SeasonalCatalogController.h"
#include "src/presentation/settings/SettingsController.h"
#include "src/application/media/SeasonalPersonalListService.h"

namespace {
constexpr int AdaptiveSyncShutdownTimeoutMs = 5'000;

void RetainAdaptiveSyncRuntimeForProcessExit(std::unique_ptr<AdaptiveSyncRuntime> runtime) {
    // Intentionally outlive application teardown: destroying a timed-out runtime would release live worker state.
    static auto *retainedRuntimes = new std::vector<std::unique_ptr<AdaptiveSyncRuntime>>;
    retainedRuntimes->push_back(std::move(runtime));
}
}

bool persistHomeSortPreference(ApplicationContext &context, const QString &key, QString &error) {
    error.clear();
    if (!context.userPreferencesRepository) {
        error = QStringLiteral("Home ordering preferences are unavailable.");
        return false;
    }

    UserPreferences updated = context.userPreferences;
    updated.homeSortKey = key;
    if (!context.userPreferencesRepository->replace(updated, error)) return false;
    context.userPreferences = std::move(updated);
    return true;
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    auto context = createApplicationContext();
    QString translationError;
    if (!TranslationLoader::Install(app, context.userPreferences.languageKey, translationError)
        && context.logger) {
        context.logger->warning(LogCategory::Configuration, translationError);
    }
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&context]() {
        if (context.adaptiveSync) {
            context.adaptiveSync->shutdown();
            if (!context.adaptiveSync->isStopped()) {
                RetainAdaptiveSyncRuntimeForProcessExit(std::move(context.adaptiveSync));
            }
        }
        if (context.localLibraryScan) {
            context.localLibraryScan->shutdown();
        }
        if (context.localLibraryRecognition) {
            context.localLibraryRecognition->shutdown();
        }
        if (context.initialSync) {
            context.initialSync->shutdown();
        }
        if (context.coverCoordinator) {
            context.coverCoordinator->disconnect();
        }
        if (context.logger) {
            context.logger->stop();
        }
    });
    HomeScreenController homeController(context.mediaRepository.get(), context.coverCoordinator.get(),
                                        context.userPreferences.coverQuality, context.initializationError);
    homeController.SetLocalEpisodeServices(context.localEpisodeReader.get(), context.localFileOpener.get());
    SeasonalPersonalListService personalLists(context.mediaRepository.get(), context.mediaRepository.get(),
                                              context.mediaRepository.get());
    SeasonalCatalogController seasonalCatalogController(context.seasonalCatalogCoordinator.get(),
                                                        context.userPreferences.coverQuality,
                                                        context.userPreferences.preferredTitleKey, nullptr,
                                                        &personalLists);
    seasonalCatalogController.ConfigureIncludeAdultContent(context.userPreferences.includeAdultContent);
    seasonalCatalogController.ConfigureScoreScale(context.userPreferences.scoreMinimum,
                                                  context.userPreferences.scoreMaximum,
                                                  context.userPreferences.scoreStep);
    homeController.ConfigureScoreScale(context.userPreferences.scoreMinimum,
                                       context.userPreferences.scoreMaximum,
                                       context.userPreferences.scoreStep);
    homeController.ConfigureCardStatusPresentation(context.userPreferences.cardStatusPresentation);
    homeController.ConfigurePreferredTitle(context.userPreferences.preferredTitleKey);
    homeController.ConfigureInitialSort(context.userPreferences.homeSortKey);
    SettingsController settingsController(context.userPreferencesRepository.get(),
                                          context.userPreferences);
    settingsController.SetScanCoordinator(context.localLibraryScan.get());
    if (context.adaptiveSync) {
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::InitializationFailed,
                         &homeController, &HomeScreenController::notifySynchronizationFailed);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::InitializationFailed,
                         &settingsController, &SettingsController::notifySynchronizationFailed);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskFailed,
                         &homeController,
                         [&homeController](const SyncPartition, const QString &error) {
                             homeController.notifySynchronizationFailed(error);
                         });
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskFailed,
                         &settingsController,
                         [&settingsController](const SyncPartition, const QString &error) {
                             settingsController.notifySynchronizationFailed(error);
                         });
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskFailed,
                         &app, [&context](const SyncPartition partition, const QString &error) {
                             if (context.logger) {
                                 context.logger->warning(LogCategory::Sync,
                                     QStringLiteral("Background synchronization for %1 failed: %2")
                                         .arg(ToString(partition), error));
                             }
                         });
        const auto schedulingError = context.schedulingInitializationError();
        if (!schedulingError.isEmpty()) homeController.notifySynchronizationFailed(schedulingError);
    }
    QObject::connect(&homeController, &HomeScreenController::sortPreferenceChanged,
                     [&context, &settingsController](const QString &key) {
        QString error;
        if (persistHomeSortPreference(context, key, error)) {
            settingsController.ApplyExternalHomeSortKey(key);
        } else if (context.logger) {
            context.logger->warning(LogCategory::Configuration, error);
        }
    });
    homeController.reload();
    if (context.localLibraryRecognition) {
        QObject::connect(context.localLibraryRecognition.get(),
                         &LocalLibraryRecognitionCoordinator::batchPersisted,
                         &homeController,
                         &HomeScreenController::RefreshLocalEpisode);
        QObject::connect(context.localLibraryRecognition.get(),
                         &LocalLibraryRecognitionCoordinator::completed,
                         &homeController,
                         [&homeController](qsizetype, qsizetype, qsizetype, qsizetype) {
                             homeController.RefreshLocalEpisode();
                         });
    }
    QObject::connect(&seasonalCatalogController, &SeasonalCatalogController::personalListSaved,
                     &homeController, &HomeScreenController::reload);

    if (context.initialSync) {
        context.initialSync->configureAutomaticSynchronization(
            false,
            context.userPreferences.synchronizationIntervalMs);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::started,
                         &homeController, &HomeScreenController::notifySynchronizationStarted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::completed,
                         &homeController, &HomeScreenController::notifySynchronizationCompleted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::failed,
                         &homeController, &HomeScreenController::notifySynchronizationFailed);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::started,
                         &settingsController, &SettingsController::notifySynchronizationStarted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::completed,
                         &settingsController, &SettingsController::notifySynchronizationCompleted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::failed,
                         &settingsController, &SettingsController::notifySynchronizationFailed);
        if (context.adaptiveSync) {
            const auto startBackgroundScheduling = [&context] {
                if (context.adaptiveSync) context.adaptiveSync->start();
            };
            QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::completed,
                             &app, startBackgroundScheduling);
            QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::failed,
                             &app, startBackgroundScheduling);
        }
        QObject::connect(&settingsController, &SettingsController::synchronizationRequested,
                         context.initialSync.get(), &InitialSyncCoordinator::start);
        if (context.userPreferences.synchronizationEnabled) context.initialSync->start();
    }

    QObject::connect(&settingsController, &SettingsController::preferencesApplied,
                     [&homeController, &seasonalCatalogController, &context](const UserPreferences &preferences) {
        context.userPreferences = preferences;
        homeController.ConfigureScoreScale(preferences.scoreMinimum,
                                           preferences.scoreMaximum,
                                           preferences.scoreStep);
        homeController.ConfigureCoverQuality(preferences.coverQuality);
        homeController.ConfigureCardStatusPresentation(preferences.cardStatusPresentation);
        homeController.ConfigurePreferredTitle(preferences.preferredTitleKey);
        seasonalCatalogController.ConfigureCoverQuality(preferences.coverQuality);
        seasonalCatalogController.ConfigurePreferredTitle(preferences.preferredTitleKey);
        seasonalCatalogController.ConfigureIncludeAdultContent(preferences.includeAdultContent);
        seasonalCatalogController.ConfigureScoreScale(preferences.scoreMinimum,
                                                      preferences.scoreMaximum,
                                                      preferences.scoreStep);
        if (context.initialSync) {
            context.initialSync->configureAutomaticSynchronization(
                false, preferences.synchronizationIntervalMs);
        }
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &homeController);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);
    engine.rootContext()->setContextProperty(QStringLiteral("seasonalCatalogController"), &seasonalCatalogController);

    bool shutdownRequested = false;
    int exitCode = 0;
    QTimer shutdownPoll;
    shutdownPoll.setInterval(10);
    QObject::connect(&shutdownPoll, &QTimer::timeout, &app, [&] {
        if (!context.adaptiveSync || context.adaptiveSync->isStopped()) {
            shutdownPoll.stop();
            app.exit(exitCode);
        }
    });
    const auto requestShutdown = [&](int code) {
        if (shutdownRequested) {
            if (code != 0) exitCode = code;
            return;
        }
        shutdownRequested = true;
        exitCode = code;
        if (!context.adaptiveSync || context.adaptiveSync->isStopped()) {
            app.exit(exitCode);
            return;
        }
        context.adaptiveSync->shutdown();
        shutdownPoll.start();
        QTimer::singleShot(AdaptiveSyncShutdownTimeoutMs, &app, [&] {
            if (context.adaptiveSync && !context.adaptiveSync->isStopped()) {
                RetainAdaptiveSyncRuntimeForProcessExit(std::move(context.adaptiveSync));
            }
            app.exit(exitCode);
        });
    };
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [&requestShutdown] { requestShutdown(-1); }, Qt::QueuedConnection);
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, &app,
                     [&requestShutdown] { requestShutdown(0); });
    engine.loadFromModule("HaikenAnime", "Main");
    if (!engine.rootObjects().isEmpty()) scheduleStartupLibraryScan(context, &settingsController);

    return app.exec();
}
