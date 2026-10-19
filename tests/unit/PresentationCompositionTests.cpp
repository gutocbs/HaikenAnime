#include <QQmlContext>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/app/AniListOAuthCallbackReceiver.h"
#include "../../src/app/ApplicationComposition.h"
#include "../../src/app/ApplicationShutdown.h"
#include "../../src/app/PresentationComposition.h"

namespace {
ApplicationCompositionOptions OptionsFor(const QTemporaryDir &directory) {
    ApplicationCompositionOptions options;
    options.databasePath = directory.filePath(QStringLiteral("library.sqlite"));
    return options;
}

QString ReceiverName() {
    return QStringLiteral("HaikenAnime.PresentationCompositionTests.%1").arg(QCoreApplication::applicationPid());
}
}

/// Characterizes the wiring that main.cpp used to own: controllers react to the application services.
class PresentationCompositionTests final : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void homeSortChangeIsStoredInTheUserPreferences();
    void completedUserListSynchronizationReloadsTheHome();
    void exposesEveryControllerUnderItsQmlName();
    void shutdownStopsTheServicesOnlyOnce();
};

void PresentationCompositionTests::homeSortChangeIsStoredInTheUserPreferences() {
    QTemporaryDir directory;
    auto context = createApplicationContext(OptionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    AniListOAuthCallbackReceiver receiver(ReceiverName());
    QObject lifetime;
    const auto presentation = CreatePresentationContext(context, receiver, lifetime);

    presentation->home->SetSort(QStringLiteral("title_desc"));

    QCOMPARE(context.userPreferences.homeSortKey, QStringLiteral("title_desc"));
    UserPreferences stored;
    bool found = false;
    QString error;
    QVERIFY2(context.userPreferencesRepository->read(stored, found, error), qPrintable(error));
    QVERIFY(found);
    QCOMPARE(stored.homeSortKey, QStringLiteral("title_desc"));
}

void PresentationCompositionTests::completedUserListSynchronizationReloadsTheHome() {
    QTemporaryDir directory;
    auto context = createApplicationContext(OptionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    AniListOAuthCallbackReceiver receiver(ReceiverName());
    QObject lifetime;
    const auto presentation = CreatePresentationContext(context, receiver, lifetime);
    presentation->home->Reload();
    QCOMPARE(presentation->home->mediaCount(), 0);

    Media media;
    media.Id = 7;
    media.Name = QStringLiteral("Frieren");
    media.ListStatus = UserListStatus::Current;
    QString error;
    QVERIFY2(context.mediaRepository->upsert({media}, error), qPrintable(error));
    emit context.adaptiveSync->BackgroundTaskCompleted(SyncPartition::UserList);

    QCOMPARE(presentation->home->mediaCount(), 1);
}

void PresentationCompositionTests::exposesEveryControllerUnderItsQmlName() {
    QTemporaryDir directory;
    auto context = createApplicationContext(OptionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    AniListOAuthCallbackReceiver receiver(ReceiverName());
    QObject lifetime;
    const auto presentation = CreatePresentationContext(context, receiver, lifetime);
    QQmlEngine engine;

    ExposeToQml(*presentation, *engine.rootContext());

    const auto exposed = [&engine](const char *name) {
        return engine.rootContext()->contextProperty(QString::fromLatin1(name)).value<QObject *>();
    };
    QCOMPARE(exposed("homeController"), presentation->home.get());
    QCOMPARE(exposed("settingsController"), presentation->settings.get());
    QCOMPARE(exposed("synchronizationStatus"), presentation->synchronizationStatus.get());
    QCOMPARE(exposed("seasonalCatalogController"), presentation->seasonalCatalog.get());
    QCOMPARE(exposed("pluginSettingsController"), presentation->pluginSettings.get());
    QCOMPARE(exposed("recognitionController"), presentation->recognition.get());
    QCOMPARE(exposed("nowWatchingController"), presentation->nowWatching.get());
    QVERIFY(presentation->nowWatching);
}

void PresentationCompositionTests::shutdownStopsTheServicesOnlyOnce() {
    QTemporaryDir directory;
    auto context = createApplicationContext(OptionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));

    ShutdownApplication(context);
    QVERIFY(context.servicesShutdown);
    ShutdownApplication(context);

    QVERIFY(context.servicesShutdown);
    QVERIFY(context.adaptiveSync->isStopped());
}

QTEST_MAIN(PresentationCompositionTests)
#include "PresentationCompositionTests.moc"
