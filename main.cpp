#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPointer>
#include <QTest>
#include "tests/listmanagertests.h"

#include "mainclass.h"
void executeTests();
int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setOrganizationName("Haiken");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("Haiken App");
    executeTests();
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
    ListManagerTests listmanager;
    QTest::qExec(&listmanager);
}
