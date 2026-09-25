#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/app/ApplicationComposition.h"
#include "src/presentation/home/HomeScreenController.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    auto context = createApplicationContext();
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&context]() {
        if (context.initialSync) {
            context.initialSync->shutdown();
        }
        if (context.logger) {
            context.logger->stop();
        }
    });
    HomeScreenController homeController(context.mediaRepository.get(), context.initializationError);
    homeController.reload();

    if (context.initialSync) {
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::started,
                         &homeController, &HomeScreenController::notifySynchronizationStarted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::completed,
                         &homeController, &HomeScreenController::notifySynchronizationCompleted);
        QObject::connect(context.initialSync.get(), &InitialSyncCoordinator::failed,
                         &homeController, &HomeScreenController::notifySynchronizationFailed);
        context.initialSync->start();
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &homeController);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("HaikenAnime", "Main");

    return app.exec();
}
