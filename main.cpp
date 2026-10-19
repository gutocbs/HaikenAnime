#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/app/AniListOAuthCallbackReceiver.h"
#include "src/app/ApplicationComposition.h"
#include "src/app/ApplicationShutdown.h"
#include "src/app/PresentationComposition.h"
#include "src/app/TranslationLoader.h"
#include "src/infrastructure/anilist/WindowsUrlProtocolRegistrar.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/HaikenAnime.png")));
    app.setQuitOnLastWindowClosed(false);

    auto context = createApplicationContext();
    AniListOAuthCallbackReceiver oauthCallbackReceiver(
        QStringLiteral("HaikenAnime.AniListOAuthCallback"));
    oauthCallbackReceiver.setAuditLogger(LogSink(context.logger.get(), LogCategory::Application));
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
    const auto presentation = CreatePresentationContext(context, oauthCallbackReceiver, app);
    QQmlApplicationEngine engine;
    ExposeToQml(*presentation, *engine.rootContext());

    int exitCode = 0;
    const auto requestShutdown = [&](int code) {
        if (code != 0) exitCode = code;
        app.exit(exitCode);
    };
    // aboutToQuit is the single exit path for QML failure, window close, and direct quit().
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&context] { ShutdownApplication(context); });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [&requestShutdown] { requestShutdown(-1); }, Qt::QueuedConnection);
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, &app,
                     [&requestShutdown] { requestShutdown(0); });
    engine.loadFromModule("HaikenAnime", "Main");
    if (!engine.rootObjects().isEmpty()) scheduleStartupLibraryScan(context, presentation->settings.get());

    return app.exec();
}
