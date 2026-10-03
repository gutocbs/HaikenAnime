#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QThread>
#include <QStandardPaths>
#include <QTimer>
#include <QDate>

#include "../../src/app/ApplicationComposition.h"
#include "../../src/presentation/home/HomeScreenController.h"
#include "../../src/presentation/settings/SettingsController.h"

namespace {
ApplicationCompositionOptions optionsFor(const QTemporaryDir &directory) {
    ApplicationCompositionOptions options;
    options.databasePath = directory.filePath("library.sqlite");
    return options;
}
void writeFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("fixture") != 7) qFatal("Cannot write fixture");
}
QString alteredConfiguration(const QTemporaryDir &directory, const QString &key, bool remove) {
    QFile resource(":/sqlite/queries/sqlite-queries.json");
    if (!resource.open(QIODevice::ReadOnly)) qFatal("Cannot read configuration");
    auto root = QJsonDocument::fromJson(resource.readAll()).object();
    auto queries = root.value("queries").toObject();
    if (remove && key == "upsertLocalFile") queries.insert(key, 42);
    else if (remove) queries.remove(key);
    else queries.insert(key, ":/does-not-exist.sql");
    root.insert("queries", queries);
    const auto path = directory.filePath("queries.json");
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly)) qFatal("Cannot write configuration");
    output.write(QJsonDocument(root).toJson());
    return path;
}
QString scanLogPath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(
        QStringLiteral("logs/haikenanime-%1.log").arg(QDate::currentDate().toString(Qt::ISODate)));
}
}

class LocalLibraryScanCompositionTests final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void persistedPreferencesOverrideDefaultsAfterRestart();
    void invalidPreferredTitleFallsBackAndLogsConfigurationWarning();
    void restartPreservesExternalHomeSortForControllerFallback();
    void startupUsesPersistedSnapshotAndSchedulesOnce();
    void disabledAutomaticRecognitionDoesNotScheduleStartupScan();
    void manualRootChangeRetainsOldInventory();
    void scannerQueryFailureDoesNotDisableMedia_data();
    void scannerQueryFailureDoesNotDisableMedia();
    void scannerQueryFailureIsLoggedNonfatally();
    void startupCleanupRemovesAbandonedCoverTemporaryWithoutTouchingCache();
    void workerProductsReleaseConnectionsAndShutdownBeforeDatabase();
    void destroyedLifetimeCancelsScheduledStartup();
};

void LocalLibraryScanCompositionTests::persistedPreferencesOverrideDefaultsAfterRestart() {
    QTemporaryDir directory;
    const auto options = optionsFor(directory);
    {
        auto context = createApplicationContext(options);
        QVERIFY2(context.isReady(), qPrintable(context.initializationError));
        QVERIFY(context.userPreferencesRepository);
        auto preferences = context.userPreferences;
        preferences.libraryRoot = directory.path();
        preferences.scanExtensions = {".webm"};
        preferences.homeSortKey = QStringLiteral("title_desc");
        preferences.preferredTitleKey = QStringLiteral("english");
        QString error;
        QVERIFY2(context.userPreferencesRepository->replace(preferences, error), qPrintable(error));
    }
    auto restarted = createApplicationContext(options);
    QVERIFY(restarted.isReady());
    QCOMPARE(restarted.userPreferences.libraryRoot, directory.path());
    QCOMPARE(restarted.userPreferences.scanExtensions, QStringList({".webm"}));
    QCOMPARE(restarted.userPreferences.homeSortKey, QStringLiteral("title_desc"));
    QCOMPARE(restarted.userPreferences.preferredTitleKey, QStringLiteral("english"));
    QVERIFY(restarted.localLibraryScan);
}

void LocalLibraryScanCompositionTests::invalidPreferredTitleFallsBackAndLogsConfigurationWarning() {
    QTemporaryDir directory;
    const auto options = optionsFor(directory);
    {
        auto context = createApplicationContext(options);
        QVERIFY2(context.isReady(), qPrintable(context.initializationError));
        auto preferences = context.userPreferences;
        preferences.coverQuality = CoverQuality::Large;
        QString error;
        QVERIFY2(context.userPreferencesRepository->replace(preferences, error), qPrintable(error));
        QSqlQuery query(context.database->connection());
        QVERIFY(query.exec(QStringLiteral(
            "UPDATE user_preferences SET preferred_title_key = 'obsolete' WHERE id = 1")));
    }

    QFile::remove(scanLogPath());
    auto restarted = createApplicationContext(options);
    QVERIFY2(restarted.isReady(), qPrintable(restarted.initializationError));
    QCOMPARE(restarted.userPreferences.preferredTitleKey, QStringLiteral("romaji"));
    QCOMPARE(restarted.userPreferences.coverQuality, CoverQuality::Large);
    restarted.logger->stop();

    QFile log(scanLogPath());
    QVERIFY(log.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto entries = QString::fromUtf8(log.readAll());
    QVERIFY(entries.contains(QStringLiteral(
        "[WARN] [Configuration] Stored preferred title key 'obsolete' is unsupported; "
        "falling back to 'romaji'.")));
}

void LocalLibraryScanCompositionTests::restartPreservesExternalHomeSortForControllerFallback() {
    QTemporaryDir directory;
    const auto options = optionsFor(directory);
    {
        auto context = createApplicationContext(options);
        QVERIFY2(context.isReady(), qPrintable(context.initializationError));
        auto preferences = context.userPreferences;
        preferences.libraryRoot = directory.path();
        preferences.scanExtensions = {QStringLiteral(".webm")};
        preferences.homeSortKey = QStringLiteral("remote_rank");
        QString error;
        QVERIFY2(context.userPreferencesRepository->replace(preferences, error), qPrintable(error));
    }

    auto restarted = createApplicationContext(options);
    QVERIFY2(restarted.isReady(), qPrintable(restarted.initializationError));
    QCOMPARE(restarted.userPreferences.libraryRoot, directory.path());
    QCOMPARE(restarted.userPreferences.scanExtensions, QStringList({QStringLiteral(".webm")}));
    QCOMPARE(restarted.userPreferences.homeSortKey, QStringLiteral("remote_rank"));

    HomeScreenController controller(*restarted.mediaRepository);
    controller.ConfigureBrowseOptions(
        {QVariantMap{{QStringLiteral("key"), QStringLiteral("anime")},
                     {QStringLiteral("label"), QStringLiteral("Anime")}}},
        {QVariantMap{{QStringLiteral("key"), QStringLiteral("all")},
                     {QStringLiteral("label"), QStringLiteral("All")}}},
        {QVariantMap{{QStringLiteral("key"), QStringLiteral("progress")},
                     {QStringLiteral("label"), QStringLiteral("Progress")}},
         QVariantMap{{QStringLiteral("key"), QStringLiteral("title_asc")},
                     {QStringLiteral("label"), QStringLiteral("Title")}}});
    controller.ConfigureInitialSort(restarted.userPreferences.homeSortKey);
    QCOMPARE(controller.activeSort(), QStringLiteral("progress"));
}

void LocalLibraryScanCompositionTests::startupUsesPersistedSnapshotAndSchedulesOnce() {
    QTemporaryDir directory;
    writeFile(directory.filePath("selected.webm"));
    writeFile(directory.filePath("excluded.mkv"));
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY(context.localLibraryScan);
    context.userPreferences.libraryRoot = directory.path();
    context.userPreferences.scanExtensions = {".webm"};
    SettingsController controller(context.userPreferencesRepository.get(), context.userPreferences);
    controller.SetScanCoordinator(context.localLibraryScan.get());
    controller.SetLibraryRoot(directory.filePath("unsaved"));
    QVERIFY(controller.dirty());
    QSignalSpy started(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::started);
    QSignalSpy completed(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::completed);
    QVERIFY(scheduleStartupLibraryScan(context, &controller));
    QVERIFY(!scheduleStartupLibraryScan(context, &controller));
    QCOMPARE(started.count(), 0);
    QTRY_COMPARE(completed.count(), 1);
    QCOMPARE(started.count(), 1);
    QCOMPARE(started.first().first().toString(), directory.path());
    QCOMPARE(controller.scanCandidateCount(), qsizetype(1));
    QVERIFY(controller.scanErrorMessage().isEmpty());
    QSqlQuery query(context.database->connection());
    QVERIFY(query.exec("SELECT relative_path FROM local_files"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("selected.webm"));
    QVERIFY(!query.next());
}

void LocalLibraryScanCompositionTests::disabledAutomaticRecognitionDoesNotScheduleStartupScan() {
    QTemporaryDir directory;
    auto context = createApplicationContext(optionsFor(directory));
    context.userPreferences.automaticLocalFileRecognition = false;
    QObject lifetime;
    QVERIFY(!scheduleStartupLibraryScan(context, &lifetime));
}

void LocalLibraryScanCompositionTests::manualRootChangeRetainsOldInventory() {
    QTemporaryDir directory, oldRoot, newRoot;
    writeFile(oldRoot.filePath("old.mkv"));
    writeFile(newRoot.filePath("new.mkv"));
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY(context.localLibraryScan);
    context.userPreferences.libraryRoot = oldRoot.path();
    SettingsController controller(context.userPreferencesRepository.get(), context.userPreferences);
    controller.SetScanCoordinator(context.localLibraryScan.get());
    QSignalSpy completed(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::completed);
    QVERIFY(scheduleStartupLibraryScan(context, &controller));
    QTRY_COMPARE(completed.count(), 1);
    controller.SetLibraryRoot(newRoot.path());
    controller.Save();
    QVERIFY(!controller.dirty());
    controller.ScanNow();
    QTRY_COMPARE(completed.count(), 2);
    QSqlQuery query(context.database->connection());
    QVERIFY(query.exec("SELECT relative_path, available FROM local_files ORDER BY relative_path"));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("new.mkv"));
    QCOMPARE(query.value(1).toInt(), 1);
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toString(), QStringLiteral("old.mkv"));
    QCOMPARE(query.value(1).toInt(), 1);
    QVERIFY(!query.next());
}

void LocalLibraryScanCompositionTests::scannerQueryFailureDoesNotDisableMedia_data() {
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("remove");
    QTest::newRow("missing-entry") << QString("beginLibraryScan") << true;
    QTest::newRow("invalid-entry-type") << QString("upsertLocalFile") << true;
    QTest::newRow("missing-resource") << QString("completeLibraryScan") << false;
}

void LocalLibraryScanCompositionTests::scannerQueryFailureDoesNotDisableMedia() {
    QFETCH(QString, key);
    QFETCH(bool, remove);
    QTemporaryDir directory;
    auto options = optionsFor(directory);
    options.queryConfigurationPath = alteredConfiguration(directory, key, remove);
    auto context = createApplicationContext(options);
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    QVERIFY(context.mediaRepository);
    QVERIFY(context.initialSync);
    QVERIFY(context.coverCoordinator);
    QVERIFY(context.localLibraryScan);
    SettingsController controller(context.userPreferencesRepository.get(), context.userPreferences);
    controller.SetScanCoordinator(context.localLibraryScan.get());
    QSignalSpy failed(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::failed);
    QVERIFY(scheduleStartupLibraryScan(context, &controller));
    QTRY_COMPARE(failed.count(), 1);
    QVERIFY(!controller.scanErrorMessage().isEmpty());
    QVERIFY(!controller.scanRunning());
    QVERIFY(context.isReady());
}

void LocalLibraryScanCompositionTests::scannerQueryFailureIsLoggedNonfatally() {
    QFile::remove(scanLogPath());
    QTemporaryDir directory;
    auto options = optionsFor(directory);
    options.queryConfigurationPath = alteredConfiguration(directory, QStringLiteral("beginLibraryScan"), true);
    auto context = createApplicationContext(options);
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    SettingsController controller(context.userPreferencesRepository.get(), context.userPreferences);
    controller.SetScanCoordinator(context.localLibraryScan.get());
    QSignalSpy failed(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::failed);
    QVERIFY(scheduleStartupLibraryScan(context, &controller));
    QTRY_COMPARE(failed.count(), 1);
    context.logger->stop();
    QFile file(scanLogPath());
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto entries = QString::fromUtf8(file.readAll());
    QVERIFY(entries.contains(QStringLiteral("[ERROR] [LocalLibrary] Local library scan completed: outcome=failed")));
    QVERIFY(entries.contains(QStringLiteral("Local library scan query configuration is incomplete.")));
    QVERIFY(context.isReady());
}

void LocalLibraryScanCompositionTests::startupCleanupRemovesAbandonedCoverTemporaryWithoutTouchingCache() {
    const QString temporaryRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                      .filePath("HaikenAnime/covers");
    const QString cacheRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
                                  .filePath("covers");
    QVERIFY(QDir().mkpath(temporaryRoot));
    QVERIFY(QDir().mkpath(cacheRoot));
    const QString abandoned = QDir(temporaryRoot).filePath("cover-composition-smoke.tmp");
    const QString persistent = QDir(cacheRoot).filePath("persistent-composition-smoke.png");
    writeFile(abandoned);
    writeFile(persistent);

    QTemporaryDir directory;
    auto context = createApplicationContext(optionsFor(directory));

    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    QVERIFY(!QFileInfo::exists(abandoned));
    QVERIFY(QFileInfo::exists(persistent));
    QVERIFY(QFile::remove(persistent));
}

void LocalLibraryScanCompositionTests::workerProductsReleaseConnectionsAndShutdownBeforeDatabase() {
    QTemporaryDir directory, root;
    for (int i = 0; i < 205; ++i) writeFile(root.filePath(QString::number(i) + ".mkv"));
    const auto before = QSqlDatabase::connectionNames();
    {
        auto context = createApplicationContext(optionsFor(directory));
        QVERIFY(context.localLibraryScan);
        const auto uiConnections = QSqlDatabase::connectionNames();
        context.userPreferences.libraryRoot = root.path();
        QObject lifetime;
        QSignalSpy started(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::started);
        QVERIFY(scheduleStartupLibraryScan(context, &lifetime));
        QTRY_COMPARE(started.count(), 1);
        context.localLibraryScan->shutdown();
        QCOMPARE(QSqlDatabase::connectionNames(), uiConnections);
        QSqlQuery query(context.database->connection());
        QVERIFY(query.exec("SELECT status FROM library_scans"));
        QVERIFY(query.next());
        QVERIFY(query.value(0).toString() == "interrupted" || query.value(0).toString() == "succeeded");
    }
    QCOMPARE(QSqlDatabase::connectionNames(), before);
    {
        auto context = createApplicationContext(optionsFor(directory));
        LocalLibraryScanRequest request;
        request.rootPath = root.path();
        request.allowedExtensions = {".mkv"};
        QVERIFY(context.localLibraryScan->start(request));
        // Context destruction joins the worker before destroying its UI database/logger.
    }
    QCOMPARE(QSqlDatabase::connectionNames(), before);
}

void LocalLibraryScanCompositionTests::destroyedLifetimeCancelsScheduledStartup() {
    QTemporaryDir directory;
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY(context.localLibraryScan);
    QSignalSpy started(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::started);
    {
        QObject lifetime;
        QVERIFY(scheduleStartupLibraryScan(context, &lifetime));
    }
    QCoreApplication::processEvents();
    QCOMPARE(started.count(), 0);
}

QTEST_GUILESS_MAIN(LocalLibraryScanCompositionTests)
#include "LocalLibraryScanCompositionTests.moc"
