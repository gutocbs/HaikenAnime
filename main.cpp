#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/app/ApplicationComposition.h"
#include "src/presentation/home/HomeScreenController.h"
#include "src/presentation/settings/SettingsController.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    auto context = createApplicationContext();
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
    homeController.ConfigureScoreScale(context.userPreferences.scoreMinimum,
                                       context.userPreferences.scoreMaximum,
                                       context.userPreferences.scoreStep);
    SettingsController settingsController(context.userPreferencesRepository.get(),
                                          context.userPreferences);
    settingsController.SetScanCoordinator(context.localLibraryScan.get());
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
                     [&homeController, &context](const UserPreferences &preferences) {
        context.userPreferences = preferences;
        homeController.ConfigureScoreScale(preferences.scoreMinimum,
                                           preferences.scoreMaximum,
                                           preferences.scoreStep);
        homeController.ConfigureCoverQuality(preferences.coverQuality);
        if (context.initialSync) {
            context.initialSync->configureAutomaticSynchronization(
                preferences.synchronizationEnabled, preferences.synchronizationIntervalMs);
        }
    });

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &homeController);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settingsController);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("HaikenAnime", "Main");
    if (!engine.rootObjects().isEmpty()) scheduleStartupLibraryScan(context, &settingsController);

    return app.exec();
}
