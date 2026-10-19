#include "PresentationComposition.h"

#include <QAbstractItemModel>
#include <QQmlContext>
#include <QTimer>

#include <utility>

#include "AniListOAuthCallbackReceiver.h"
#include "../application/anilist/AniListAccountOwnership.h"

std::unique_ptr<PresentationContext> CreatePresentationContext(ApplicationContext &context,
                                                               AniListOAuthCallbackReceiver &oauthCallbackReceiver,
                                                               QObject &lifetime) {
    auto presentation = std::make_unique<PresentationContext>();
    presentation->home = std::make_unique<HomeScreenController>(
        context.mediaRepository.get(), context.coverCoordinator.get(), context.userPreferences.coverQuality,
        context.initializationError);
    auto &homeController = *presentation->home;
    homeController.SetLocalEpisodeServices(context.localEpisodeReader.get(), context.localFileOpener.get());
    homeController.SetPersonalListChangeService(context.personalListChangeService.get());
    if (context.personalListChangeService && context.adaptiveSync) {
        context.personalListChangeService->setPendingChangesNotifier(
            [runtime = context.adaptiveSync.get(), auth = context.aniListAuthManager.get()] {
                if (runtime && auth && !auth->credentials().token.isEmpty()) {
                    runtime->requestNow(SyncPartition::PendingChanges);
                }
            });
    }
    presentation->personalLists = std::make_unique<SeasonalPersonalListService>(
        context.mediaRepository.get(), context.personalListChangeService.get());
    presentation->seasonalCatalog = std::make_unique<SeasonalCatalogController>(
        context.seasonalCatalogCoordinator.get(), context.userPreferences.coverQuality,
        context.userPreferences.preferredTitleKey, nullptr, presentation->personalLists.get());
    auto &seasonalCatalogController = *presentation->seasonalCatalog;
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
    presentation->settings = std::make_unique<SettingsController>(
        context.userPreferencesRepository.get(), context.userPreferences, context.clearLocalCache.get());
    auto &settingsController = *presentation->settings;
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
    presentation->synchronizationStatus = std::make_unique<SynchronizationStatusController>();
    auto &synchronizationStatus = *presentation->synchronizationStatus;
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
                         &lifetime, [&context](const SyncPartition partition, const QString &error) {
                             if (context.logger) {
                                 context.logger->warning(LogCategory::Sync,
                                     QStringLiteral("Background synchronization for %1 failed: %2")
                                         .arg(ToString(partition), error));
                             }
                         });
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCompleted,
                         &homeController, [&homeController](const SyncPartition partition) {
                             if (partition == SyncPartition::UserList) homeController.Reload();
                         });
        QObject::connect(context.adaptiveSync.get(), &AdaptiveSyncRuntime::BackgroundTaskCompleted,
                         &lifetime, [&context](const SyncPartition partition) {
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
        if (UpdateUserPreferences(context, [&key](UserPreferences &preferences) { preferences.homeSortKey = key; }, error)) {
            settingsController.ApplyExternalHomeSortKey(key);
        } else if (context.logger) {
            context.logger->warning(LogCategory::Configuration, error);
        }
    });
    homeController.Reload();
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
                     &homeController, &HomeScreenController::Reload);
    if (context.localLibraryRecognition) {
        // Saving a folder or alternative names re-evaluates the local files in the background; the
        // coordinator queues overlapping requests and the Home refreshes on its batch signals.
        const auto requestLocalRecognition = [&context](const QStringList &folderPaths, bool alternativeNamesChanged) {
            auto *recognition = context.localLibraryRecognition.get();
            for (const auto &folderPath : folderPaths) recognition->startForFolder(folderPath);
            if (!alternativeNamesChanged) return;
            for (const auto &root : context.userPreferences.libraryRoots) recognition->startForPending(root);
        };
        QObject::connect(&homeController, &HomeScreenController::localRecognitionRequested,
                         &homeController, requestLocalRecognition);
        QObject::connect(&seasonalCatalogController, &SeasonalCatalogController::localRecognitionRequested,
                         &homeController, requestLocalRecognition);
    }

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
        if (UpdateUserPreferences(context, [userId](UserPreferences &preferences) { preferences.lastAniListUserId = userId; }, error)) {
            settingsController.ApplyExternalLastAniListUserId(userId);
        } else if (context.logger) {
            context.logger->warning(LogCategory::Configuration, error);
        }
    };
    QObject::connect(&settingsController, &SettingsController::aniListAuthenticationSucceeded,
                     &lifetime, rememberConnectedAniListAccount);
    auto &removeAniListAccount = presentation->removeAniListAccount;
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
                if (!UpdateUserPreferences(context, [userId](UserPreferences &preferences) {
                        preferences.lastAniListUserId = userId;
                    }, error)) {
                    return false;
                }
                settingsController.ApplyExternalLastAniListUserId(userId);
                return true;
            },
            LogSink(context.logger.get(), LogCategory::Application));
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
                         &lifetime, requestAniListSynchronization);
        QObject::connect(&settingsController, &SettingsController::aniListAuthenticationSucceeded,
                         &lifetime, requestAniListSynchronization);
        if (context.userPreferences.synchronizationEnabled) {
            QTimer::singleShot(0, &lifetime, requestAniListSynchronization);
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

    auto &pluginSettingsController = presentation->pluginSettings;
    auto &recognitionController = presentation->recognition;
    auto &nowWatchingController = presentation->nowWatching;
    if (context.recognition && context.recognition->recognition && context.mediaRepository) {
        auto &recognition = *context.recognition;
        pluginSettingsController = std::make_unique<PluginSettingsController>(
            *recognition.settings, recognition.registry,
            PlexAccountServices{.authentication = recognition.plexAuthentication.get(),
                                .api = recognition.plexApi.get(),
                                .plugin = recognition.plex},
            [&recognition] { applyRecognitionSettings(recognition); });
        recognitionController = std::make_unique<EpisodeRecognitionController>(
            *recognition.history, *context.mediaRepository);
        nowWatchingController = std::make_unique<NowWatchingController>(
            recognition.recognition->activeSessions(), *context.mediaRepository,
            LogSink(context.logger.get(), LogCategory::Application, LogLevel::Warning));
        nowWatchingController->ConfigurePreferredTitle(context.userPreferences.preferredTitleKey);
        nowWatchingController->SetCoverSource([&homeController](const int mediaId) {
            return homeController.fullMediaModel()->CoverSource(mediaId);
        });
        QObject::connect(&settingsController, &SettingsController::preferencesApplied, nowWatchingController.get(),
                         [controller = nowWatchingController.get()](const UserPreferences &preferences) {
                             controller->ConfigurePreferredTitle(preferences.preferredTitleKey);
                         });
        // Automatic progress changes the library; the Home refreshes after the recognition callback returns.
        // The Home reloads the library after a sync or an edit; the screen's cached copy follows it.
        QObject::connect(homeController.fullMediaModel(), &QAbstractItemModel::modelReset, nowWatchingController.get(),
                         [controller = nowWatchingController.get(), resolver = recognition.resolver.get()] {
                             controller->InvalidateLibrary();
                             // Title matching for new sessions follows the same library reloads.
                             if (resolver) resolver->invalidate();
                         });
        *context.recognitionProgressListener = [&homeController](const Media &) {
            QTimer::singleShot(0, &homeController, [&homeController] { homeController.Reload(); });
        };
    }

    return presentation;
}

void ExposeToQml(PresentationContext &presentation, QQmlContext &qml) {
    qml.setContextProperty(QStringLiteral("homeController"), presentation.home.get());
    qml.setContextProperty(QStringLiteral("settingsController"), presentation.settings.get());
    qml.setContextProperty(QStringLiteral("synchronizationStatus"), presentation.synchronizationStatus.get());
    qml.setContextProperty(QStringLiteral("seasonalCatalogController"), presentation.seasonalCatalog.get());
    // Null when recognition could not be composed; the settings screen then hides those sections.
    qml.setContextProperty(QStringLiteral("pluginSettingsController"), presentation.pluginSettings.get());
    qml.setContextProperty(QStringLiteral("recognitionController"), presentation.recognition.get());
    qml.setContextProperty(QStringLiteral("nowWatchingController"), presentation.nowWatching.get());
}
