#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "src/infrastructure/database/SqliteDatabase.h"
#include "src/infrastructure/database/SqliteMediaRepository.h"
#include "src/infrastructure/database/SqlQueryStore.h"
#include "src/presentation/home/HomeScreenController.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    SqliteDatabase database;
    database.open();
    database.migrate();
    SqlQueryStore upsertQueryStore(QStringLiteral(":/sqlite/queries/upsert-media.sql"));
    SqlQueryStore readQueryStore(QStringLiteral(":/sqlite/queries/read-media.sql"));
    QString queryError;
    QString upsertQuery;
    QString readQuery;
    upsertQueryStore.load(upsertQuery, queryError);
    readQueryStore.load(readQuery, queryError);
    SqliteMediaRepository mediaRepository(database.connection(), upsertQuery, readQuery);
    HomeScreenController homeController(mediaRepository);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &homeController);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("HaikenAnime", "Main");

    return app.exec();
}
