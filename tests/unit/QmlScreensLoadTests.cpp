#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "../../src/app/SeasonalCatalogCoordinator.h"
#include "../../src/application/catalog/ISeasonalCatalogDataSource.h"
#include "../../src/application/configuration/IUserPreferencesRepository.h"
#include "../../src/application/media/IMediaReader.h"
#include "../../src/presentation/home/HomeScreenController.h"
#include "../../src/presentation/seasonal/SeasonalCatalogController.h"
#include "../../src/presentation/settings/SettingsController.h"
#include "../../src/presentation/sync/SynchronizationStatusController.h"

namespace {
Media SampleMedia() {
    Media media;
    media.Id = 1;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.Status = MediaStatus::Releasing;
    media.ListStatus = UserListStatus::Current;
    media.TotalChapters = 28;
    media.ConsumedChapters = 3;
    media.PersonalScore = 9;
    media.Synopsis = QStringLiteral("A mage outlives her party.");
    return media;
}

class LibraryReader final : public IMediaReader {
public:
    bool readAll(QList<Media> &media, QString &error) override {
        error.clear();
        media = {SampleMedia()};
        return true;
    }
};

class SeasonPageSource final : public ISeasonalCatalogDataSource {
public:
    bool Fetch(const SeasonalCatalogRequest &request, MediaPage &result, QString &error) override {
        error.clear();
        result = {.media = {SampleMedia()}, .hasNextPage = false, .currentPage = request.page, .totalPages = 1};
        return true;
    }
};

class PreferencesRepository final : public IUserPreferencesRepository {
public:
    bool read(UserPreferences &, bool &found, QString &error) override {
        found = false;
        error.clear();
        return true;
    }
    bool replace(const UserPreferences &, QString &error) override {
        error.clear();
        return true;
    }
};

QUrl ScreenUrl(const QString &name) {
    return QUrl::fromLocalFile(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/") + name);
}
}

/// Loads each screen with its real controller over fakes. Any QML warning (broken binding, unknown property,
/// ReferenceError) carries a ".qml:<line>" location and fails the test.
class QmlScreensLoadTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void homeLoadsWithoutQmlWarnings();
    void seasonalCatalogLoadsWithoutQmlWarnings();
    void settingsLoadsWithoutQmlWarnings();
    void settingsOpensTheLogsFolderFromTheSidebarOfEveryCategory();

private:
    static void Load(QQmlEngine &engine, const QString &screen);
};

void QmlScreensLoadTests::init() {
    QTest::failOnWarning(QRegularExpression(QStringLiteral(R"(\.qml:\d+)")));
}

void QmlScreensLoadTests::Load(QQmlEngine &engine, const QString &screen) {
    QQmlComponent component(&engine, ScreenUrl(screen));
    const std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    QCoreApplication::processEvents();
}

void QmlScreensLoadTests::homeLoadsWithoutQmlWarnings() {
    LibraryReader reader;
    HomeScreenController home(reader);
    home.Reload();
    home.SelectMedia(1);
    SynchronizationStatusController status;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("homeController"), &home);
    engine.rootContext()->setContextProperty(QStringLiteral("synchronizationStatus"), &status);
    engine.rootContext()->setContextProperty(QStringLiteral("nowWatchingController"), nullptr);

    Load(engine, QStringLiteral("Home.qml"));
}

void QmlScreensLoadTests::seasonalCatalogLoadsWithoutQmlWarnings() {
    SeasonPageSource source;
    SeasonalCatalogCoordinator coordinator(source, kSeasonalCatalogMaximumPageSize, nullptr, {},
                                           [] { return qint64{0}; }, [](qint64) {});
    SeasonalCatalogController seasonal(&coordinator);
    seasonal.SetYear(2023);
    seasonal.SetSeason(QStringLiteral("FALL"));
    seasonal.SelectMedia(1);
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("seasonalCatalogController"), &seasonal);

    Load(engine, QStringLiteral("SeasonalCatalogScreen.qml"));
}

void QmlScreensLoadTests::settingsLoadsWithoutQmlWarnings() {
    PreferencesRepository repository;
    UserPreferences preferences;
    preferences.libraryRoots = {QStringLiteral("D:/Anime")};
    SettingsController settings(&repository, preferences);
    SynchronizationStatusController status;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("synchronizationStatus"), &status);
    // Recognition could not start: the screen hides plugins and history instead of failing.
    engine.rootContext()->setContextProperty(QStringLiteral("pluginSettingsController"), nullptr);
    engine.rootContext()->setContextProperty(QStringLiteral("recognitionController"), nullptr);

    Load(engine, QStringLiteral("SettingsScreen.qml"));
}

void QmlScreensLoadTests::settingsOpensTheLogsFolderFromTheSidebarOfEveryCategory() {
    PreferencesRepository repository;
    SettingsController settings(&repository, UserPreferences{});
    QTemporaryDir logs;
    QVERIFY(logs.isValid());
    QUrl openedUrl;
    settings.SetLogsFolder(logs.path(), [&openedUrl](const QUrl &url) {
        openedUrl = url;
        return true;
    });
    SynchronizationStatusController status;
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("settingsController"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("synchronizationStatus"), &status);
    engine.rootContext()->setContextProperty(QStringLiteral("pluginSettingsController"), nullptr);
    engine.rootContext()->setContextProperty(QStringLiteral("recognitionController"), nullptr);
    QQmlComponent component(&engine, ScreenUrl(QStringLiteral("SettingsScreen.qml")));
    const std::unique_ptr<QObject> screen(component.create());
    QVERIFY2(screen, qPrintable(component.errorString()));

    // The screen opens on the overview, so a visible button there lives outside any category.
    QCOMPARE(screen->property("activeSection").toString(), QStringLiteral("overview"));
    auto *button = screen->findChild<QObject *>(QStringLiteral("openLogsFolderButton"));
    QVERIFY(button);
    QVERIFY(button->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));

    QCOMPARE(openedUrl, QUrl::fromLocalFile(logs.path()));
}

QTEST_MAIN(QmlScreensLoadTests)
#include "QmlScreensLoadTests.moc"
