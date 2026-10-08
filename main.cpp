#include <QGuiApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>

#include <memory>
#include <utility>

#include "src/app/ApplicationComposition.h"
#include "src/app/TranslationLoader.h"
#include "src/presentation/home/HomeScreenController.h"
#include "src/presentation/seasonal/SeasonalCatalogController.h"
#include "src/presentation/settings/SettingsController.h"
#include "src/application/media/SeasonalPersonalListService.h"

namespace {
constexpr qint64 AdaptiveSyncShutdownTimeoutMs = 5'000;
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
    bool servicesShutdown = false;
    const auto shutdownServices = [&context, &servicesShutdown] {
        if (servicesShutdown) return;
        servicesShutdown = true;
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
    };
    const auto waitForAdaptiveSyncStop = [&context] {
        if (!context.adaptiveSync || context.adaptiveSync->isStopped()) return true;

        context.adaptiveSync->shutdown();
        QElapsedTimer elapsed;
        elapsed.start();
        while (!context.adaptiveSync->isStopped() && !elapsed.hasExpired(AdaptiveSyncShutdownTimeoutMs)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
            QThread::msleep(1);
        }
        return context.adaptiveSync->isStopped();
    };
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
                                          context.userPreferences,
                                          context.clearLocalCache.get());
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
            context.initialSync->configureEnabledUserLists(preferences.enabledUserLists);
            context.initialSync->configureAutomaticSynchronization(
                false, preferences.synchronizationIntervalMs);
        }
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &homeController);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);
    engine.rootContext()->setContextProperty(QStringLiteral("seasonalCatalogController"), &seasonalCatalogController);

    int exitCode = 0;
    const auto requestShutdown = [&](int code) {
        if (code != 0) exitCode = code;
        app.exit(exitCode);
    };
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&] {
        // This signal is the single exit path for QML failure, window close, and direct quit().
        // On a timeout, ApplicationContext destruction still owns the runtime and joins its thread;
        // dependent services intentionally remain alive until that has happened.
        if (!waitForAdaptiveSyncStop()) {
            if (context.logger) {
                context.logger->warning(LogCategory::Sync,
                                        QStringLiteral("Adaptive synchronization did not stop before shutdown timeout."));
            }
            return;
        }
        shutdownServices();
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [&requestShutdown] { requestShutdown(-1); }, Qt::QueuedConnection);
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, &app,
                     [&requestShutdown] { requestShutdown(0); });
    engine.loadFromModule("HaikenAnime", "Main");
    if (!engine.rootObjects().isEmpty()) scheduleStartupLibraryScan(context, &settingsController);

    return app.exec();
}
