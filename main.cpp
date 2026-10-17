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
#include "src/application/anilist/AniListAccountOwnership.h"
#include "src/application/anilist/RemoveAniListAccountUseCase.h"
#include "src/app/AniListOAuthCallbackReceiver.h"
#include "src/app/TranslationLoader.h"
#include "src/infrastructure/anilist/WindowsUrlProtocolRegistrar.h"
#include "src/presentation/home/HomeScreenController.h"
#include "src/presentation/seasonal/SeasonalCatalogController.h"
#include "src/presentation/settings/SettingsController.h"
#include "src/presentation/sync/SynchronizationStatusController.h"
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

bool persistLastAniListUserId(ApplicationContext &context, const qint64 userId, QString &error) {
    error.clear();
    if (!context.userPreferencesRepository) {
        error = QStringLiteral("User preferences are unavailable.");
        return false;
    }

    UserPreferences updated = context.userPreferences;
    updated.lastAniListUserId = userId;
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
    if (context.personalListChangeService && context.adaptiveSync) {
        context.personalListChangeService->setPendingChangesNotifier(
            [runtime = context.adaptiveSync.get(), auth = context.aniListAuthManager.get()] {
                if (runtime && auth && !auth->credentials().token.isEmpty()) {
                    runtime->requestNow(SyncPartition::PendingChanges);
                }
            });
    }
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
    SynchronizationStatusController synchronizationStatus;
    if (context.adaptiveSync) {
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::InitializationFailed,
                         &synchronizationStatus,
                         &SynchronizationStatusController::HandleInitializationFailed);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskStarted,
                         &synchronizationStatus, &SynchronizationStatusController::HandleStarted);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskProgress,
                         &synchronizationStatus, &SynchronizationStatusController::HandleProgress);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskFailed,
                         &synchronizationStatus, &SynchronizationStatusController::HandleFailed);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCancelled,
                         &synchronizationStatus, &SynchronizationStatusController::HandleCancelled);
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCompleted,
                         &synchronizationStatus, &SynchronizationStatusController::HandleCompleted);
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
                         &app, [&context](const SyncPartition partition) {
                             if (context.logger) {
                                 context.logger->info(
                                     LogCategory::Sync,
                                     QStringLiteral("Background synchronization for %1 completed.")
                                         .arg(ToString(partition)));
                             }
                         });
        const auto schedulingError = context.schedulingInitializationError();
        if (!schedulingError.isEmpty()) {
            synchronizationStatus.HandleInitializationFailed(schedulingError);
        }
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

    if (context.personalListChangeService) {
        context.personalListChangeService->setOwnerProvider([&context]() -> qint64 {
            const qint64 connectedUserId = context.aniListAuthManager
                ? context.aniListAuthManager->credentials().userId : 0;
            return AniListAccountOwnership::ResolveOwnerUserId(
                connectedUserId, context.userPreferences.lastAniListUserId);
        });
    }
    const auto rememberConnectedAniListAccount = [&context, &settingsController] {
        const qint64 userId = AniListAccountOwnership::AccountToRemember(
            context.aniListAuthManager ? context.aniListAuthManager->credentials().userId : 0,
            context.userPreferences.lastAniListUserId);
        if (userId <= 0) return;
        QString error;
        if (persistLastAniListUserId(context, userId, error)) {
            settingsController.ApplyExternalLastAniListUserId(userId);
        } else if (context.logger) {
            context.logger->warning(LogCategory::Configuration, error);
        }
    };
    QObject::connect(&settingsController, &SettingsController::aniListAuthenticationSucceeded,
                     &app, rememberConnectedAniListAccount);
    std::unique_ptr<RemoveAniListAccountUseCase> removeAniListAccount;
    if (context.aniListAuthManager) {
        removeAniListAccount = std::make_unique<RemoveAniListAccountUseCase>(
            [&oauthCallbackReceiver] { oauthCallbackReceiver.stop(); },
            [&context](const bool suspended) {
                if (!context.aniListAuthManager) return;
                if (suspended) context.aniListAuthManager->suspendCredentials();
                else context.aniListAuthManager->resumeCredentials();
            },
            [&context](const SyncPartition partition, std::function<void()> acknowledged) {
                // Without a runtime there is no running synchronization to wait for.
                if (context.adaptiveSync) context.adaptiveSync->cancel(partition, std::move(acknowledged));
                else if (acknowledged) acknowledged();
            },
            *context.aniListAuthManager,
            [&context, &settingsController](const qint64 userId, QString &error) {
                // Edits made offline after the removal still belong to the removed account.
                if (!persistLastAniListUserId(context, userId, error)) return false;
                settingsController.ApplyExternalLastAniListUserId(userId);
                return true;
            },
            [logger = context.logger.get()](const QString &event) {
                if (logger) logger->info(LogCategory::Application, event);
            });
        settingsController.SetAniListAccountRemoval(removeAniListAccount.get());
    }
    // Accounts connected before this release (or loaded from the secret store) must also be remembered.
    rememberConnectedAniListAccount();
    if (context.adaptiveSync) {
        const auto requestAniListSynchronization = [&context] {
            if (!context.adaptiveSync) return;
            if (!context.aniListAuthManager
                || context.aniListAuthManager->credentials().token.isEmpty()) return;
            context.adaptiveSync->requestNow(SyncPartition::UserList);
            context.adaptiveSync->requestNow(SyncPartition::PendingChanges);
        };
        QObject::connect(&settingsController, &SettingsController::synchronizationRequested,
                         &app, requestAniListSynchronization);
        QObject::connect(&settingsController, &SettingsController::aniListAuthenticationSucceeded,
                         &app, requestAniListSynchronization);
        if (context.userPreferences.synchronizationEnabled) {
            QTimer::singleShot(0, &app, requestAniListSynchronization);
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
    engine.rootContext()->setContextProperty(QStringLiteral("synchronizationStatus"), &synchronizationStatus);
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
