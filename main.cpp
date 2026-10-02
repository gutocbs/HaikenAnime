#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <utility>

#include "src/app/ApplicationComposition.h"
#include "src/app/TranslationLoader.h"
#include "src/presentation/home/HomeScreenController.h"
#include "src/presentation/seasonal/SeasonalCatalogController.h"
#include "src/presentation/settings/SettingsController.h"

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

    auto context = createApplicationContext();
    QString translationError;
    if (!TranslationLoader::Install(app, context.userPreferences.languageKey, translationError)
        && context.logger) {
        context.logger->warning(LogCategory::Configuration, translationError);
    }
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&context]() {
        if (context.localLibraryScan) {
            context.localLibraryScan->shutdown();
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
    SeasonalCatalogController seasonalCatalogController(context.seasonalCatalogCoordinator.get(),
                                                        context.userPreferences.coverQuality,
                                                        context.userPreferences.preferredTitleKey);
    homeController.ConfigureScoreScale(context.userPreferences.scoreMinimum,
                                       context.userPreferences.scoreMaximum,
                                       context.userPreferences.scoreStep);
    homeController.ConfigureCardStatusPresentation(context.userPreferences.cardStatusPresentation);
    homeController.ConfigurePreferredTitle(context.userPreferences.preferredTitleKey);
    homeController.ConfigureInitialSort(context.userPreferences.homeSortKey);
    SettingsController settingsController(context.userPreferencesRepository.get(),
                                          context.userPreferences);
    settingsController.SetScanCoordinator(context.localLibraryScan.get());
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

    if (context.initialSync) {
        context.initialSync->configureAutomaticSynchronization(
            context.userPreferences.synchronizationEnabled,
            context.userPreferences.synchronizationIntervalMs);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::started,
                         &homeController, &HomeScreenController::notifySynchronizationStarted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::completed,
                         &homeController, &HomeScreenController::notifySynchronizationCompleted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::failed,
                         &homeController, &HomeScreenController::notifySynchronizationFailed);
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
        seasonalCatalogController.ConfigureCoverQuality(preferences.coverQuality);
        seasonalCatalogController.ConfigurePreferredTitle(preferences.preferredTitleKey);
        if (context.initialSync) {
            context.initialSync->configureAutomaticSynchronization(
                preferences.synchronizationEnabled, preferences.synchronizationIntervalMs);
        }
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &homeController);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);
    engine.rootContext()->setContextProperty(QStringLiteral("seasonalCatalogController"), &seasonalCatalogController);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("HaikenAnime", "Main");
    if (!engine.rootObjects().isEmpty()) scheduleStartupLibraryScan(context, &settingsController);

    return app.exec();
}
