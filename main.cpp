#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPointer>

#include "mainclass.h"
#include "tests/testmanager.h"

void executeTests();
int main(int argc, char *argv[])
{
//#if QTEST()
//    executeTests();
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("Haiken");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("Haiken App");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    MainClass cmainClass;

    engine.rootContext()->setContextProperty("mainClass", &cmainClass);

    const QUrl url(QStringLiteral("qrc:/QML/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}

void executeTests(){
    TestManager testManager;
    testManager.executeAllTests();
}
