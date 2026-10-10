#include <QGuiApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QTimer>

#include <memory>
#include <utility>

#include "src/app/ApplicationComposition.h"
#include "src/app/AniListOAuthCallbackReceiver.h"
#include "src/app/TranslationLoader.h"
#include "src/infrastructure/anilist/WindowsUrlProtocolRegistrar.h"
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
    AniListOAuthCallbackReceiver oauthCallbackReceiver(
        QStringLiteral("HaikenAnime.AniListOAuthCallback"));
    oauthCallbackReceiver.setAuditLogger([logger = context.logger.get()](const QString &event) {
        if (logger) logger->info(LogCategory::Application, event);
    });
    const auto initialOAuthCallback = AniListOAuthCallbackReceiver::callbackFromArguments(app.arguments());
    if (initialOAuthCallback.has_value()) {
        QString callbackReceiverError;
        if (oauthCallbackReceiver.start(app.arguments(), callbackReceiverError)
            != AniListOAuthCallbackReceiver::StartResult::Forwarded) {
            if (context.logger) {
                context.logger->warning(
                    LogCategory::Application,
                    QStringLiteral("AniList OAuth callback could not be forwarded: %1")
                        .arg(callbackReceiverError));
            }
        }
        return 0;
    }
    WindowsUrlProtocolRegistrar protocolRegistrar;
    QString protocolRegistrationError;
    if (!protocolRegistrar.registerProtocol(QStringLiteral("haikenanime"),
                                            QCoreApplication::applicationFilePath(),
                                            protocolRegistrationError)
        && context.logger) {
        context.logger->warning(LogCategory::Configuration, protocolRegistrationError);
    }
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
    homeController.SetPersonalListMediaWriter(context.mediaRepository.get());
    homeController.SetPersonalListChangeService(context.personalListChangeService.get());
    SeasonalPersonalListService personalLists(context.mediaRepository.get(), context.mediaRepository.get(),
                                              context.mediaRepository.get(), context.personalListChangeService.get());
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
    if (context.aniListAuthManager && context.aniListOAuthConfiguration
        && context.aniListOAuthLauncher && context.aniListViewerClient) {
        settingsController.SetAniListAuthenticationServices(
            context.aniListAuthManager.get(), *context.aniListOAuthConfiguration,
            [&context](const QUrl &url, QString &error) {
                return context.aniListOAuthLauncher->launch(url, error);
            }, context.aniListViewerClient.get(),
            [&oauthCallbackReceiver](QString &error) {
                return oauthCallbackReceiver.start({}, error)
                    == AniListOAuthCallbackReceiver::StartResult::Listening;
            },
            [&oauthCallbackReceiver] { oauthCallbackReceiver.stop(); });
    }
    QObject::connect(&oauthCallbackReceiver, &AniListOAuthCallbackReceiver::callbackReceived,
                     &settingsController, &SettingsController::HandleAniListOAuthCallback);
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
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCompleted,
                         &homeController, [&homeController](const SyncPartition partition) {
                             if (partition == SyncPartition::UserList) homeController.reload();
                         });
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCompleted,
                         &settingsController, [&settingsController](const SyncPartition partition) {
                             if (partition == SyncPartition::UserList) {
                                 settingsController.notifySynchronizationCompleted();
                             }
                         });
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCompleted,
                         &app, [&context](const SyncPartition partition) {
                             if (context.logger) {
                                 context.logger->info(
                                     LogCategory::Sync,
                                     QStringLiteral("Background synchronization for %1 completed.")
                                         .arg(ToString(partition)));
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

    if (context.adaptiveSync) {
        const auto requestUserListSynchronization = [&context, &homeController, &settingsController] {
            if (!context.adaptiveSync) return;
            homeController.notifySynchronizationStarted();
            settingsController.notifySynchronizationStarted();
            context.adaptiveSync->requestNow(SyncPartition::UserList);
        };
        QObject::connect(&settingsController, &SettingsController::synchronizationRequested,
                         &app, requestUserListSynchronization);
        QObject::connect(&settingsController, &SettingsController::aniListAuthenticationSucceeded,
                         &app, requestUserListSynchronization);
        if (context.userPreferences.synchronizationEnabled) {
            QTimer::singleShot(0, &app, requestUserListSynchronization);
        }
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
