#include <QtTest>
#include <QFile>
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
QString scanLogPath() {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(
        QStringLiteral("logs/haikenanime-%1.log").arg(QDate::currentDate().toString(Qt::ISODate)));
}
}

class ApplicationCompositionTests final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void persistedPreferencesOverrideDefaultsAfterRestart();
    void invalidPreferredTitleFallsBackAndLogsConfigurationWarning();
    void restartPreservesExternalHomeSortForControllerFallback();
    void startupUsesPersistedSnapshotAndSchedulesOnce();
    void disabledAutomaticRecognitionDoesNotScheduleStartupScan();
    void manualRootChangeRetainsOldInventory();
    void startupCleanupRemovesAbandonedCoverTemporaryWithoutTouchingCache();
    void manualCleanupUsesComposedCoverTemporaryStore();
    void workerProductsReleaseConnectionsAndShutdownBeforeDatabase();
    void destroyedLifetimeCancelsScheduledStartup();
    void composesAdaptiveSynchronization();
};

void ApplicationCompositionTests::persistedPreferencesOverrideDefaultsAfterRestart() {
    QTemporaryDir directory;
    const auto options = optionsFor(directory);
    {
        auto context = createApplicationContext(options);
        QVERIFY2(context.isReady(), qPrintable(context.initializationError));
        QVERIFY(context.userPreferencesRepository);
        auto preferences = context.userPreferences;
        preferences.libraryRoots = {directory.path()};
        preferences.scanExtensions = {".webm"};
        preferences.homeSortKey = QStringLiteral("title_desc");
        preferences.preferredTitleKey = QStringLiteral("english");
        QString error;
        QVERIFY2(context.userPreferencesRepository->replace(preferences, error), qPrintable(error));
    }
    auto restarted = createApplicationContext(options);
    QVERIFY(restarted.isReady());
    QCOMPARE(restarted.userPreferences.libraryRoots, QStringList{directory.path()});
    QCOMPARE(restarted.userPreferences.scanExtensions, QStringList({".webm"}));
    QCOMPARE(restarted.userPreferences.homeSortKey, QStringLiteral("title_desc"));
    QCOMPARE(restarted.userPreferences.preferredTitleKey, QStringLiteral("english"));
    QVERIFY(restarted.localLibraryScan);
}

void ApplicationCompositionTests::invalidPreferredTitleFallsBackAndLogsConfigurationWarning() {
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

void ApplicationCompositionTests::restartPreservesExternalHomeSortForControllerFallback() {
    QTemporaryDir directory;
    const auto options = optionsFor(directory);
    {
        auto context = createApplicationContext(options);
        QVERIFY2(context.isReady(), qPrintable(context.initializationError));
        auto preferences = context.userPreferences;
        preferences.libraryRoots = {directory.path()};
        preferences.scanExtensions = {QStringLiteral(".webm")};
        preferences.homeSortKey = QStringLiteral("remote_rank");
        QString error;
        QVERIFY2(context.userPreferencesRepository->replace(preferences, error), qPrintable(error));
    }

    auto restarted = createApplicationContext(options);
    QVERIFY2(restarted.isReady(), qPrintable(restarted.initializationError));
    QCOMPARE(restarted.userPreferences.libraryRoots, QStringList{directory.path()});
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

void ApplicationCompositionTests::startupUsesPersistedSnapshotAndSchedulesOnce() {
    QTemporaryDir directory;
    writeFile(directory.filePath("selected.webm"));
    writeFile(directory.filePath("excluded.mkv"));
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY(context.localLibraryScan);
    context.userPreferences.libraryRoots = {directory.path()};
    context.userPreferences.scanExtensions = {".webm"};
    SettingsController controller(context.userPreferencesRepository.get(), context.userPreferences);
    controller.SetScanCoordinator(context.localLibraryScan.get());
    controller.SetLibraryRoot(0, directory.filePath("unsaved"));
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

void ApplicationCompositionTests::disabledAutomaticRecognitionDoesNotScheduleStartupScan() {
    QTemporaryDir directory;
    auto context = createApplicationContext(optionsFor(directory));
    context.userPreferences.automaticLocalFileRecognition = false;
    QObject lifetime;
    QVERIFY(!scheduleStartupLibraryScan(context, &lifetime));
}

void ApplicationCompositionTests::manualRootChangeRetainsOldInventory() {
    QTemporaryDir directory, oldRoot, newRoot;
    writeFile(oldRoot.filePath("old.mkv"));
    writeFile(newRoot.filePath("new.mkv"));
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY(context.localLibraryScan);
    context.userPreferences.libraryRoots = {oldRoot.path()};
    SettingsController controller(context.userPreferencesRepository.get(), context.userPreferences);
    controller.SetScanCoordinator(context.localLibraryScan.get());
    QSignalSpy completed(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::completed);
    QVERIFY(scheduleStartupLibraryScan(context, &controller));
    QTRY_COMPARE(completed.count(), 1);
    controller.SetLibraryRoot(0, newRoot.path());
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

void ApplicationCompositionTests::startupCleanupRemovesAbandonedCoverTemporaryWithoutTouchingCache() {
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

void ApplicationCompositionTests::manualCleanupUsesComposedCoverTemporaryStore() {
    QTemporaryDir directory;
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    QVERIFY(context.coverTemporaryStore);
    QVERIFY(context.clearLocalCache);
    const QString temporaryRoot = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                      .filePath("HaikenAnime/covers");
    QVERIFY(QDir().mkpath(temporaryRoot));
    const QString temporaryPath = QDir(temporaryRoot).filePath("cover-manual-cleanup.tmp");
    writeFile(temporaryPath);

    ClearLocalCacheResult result;
    QVERIFY(context.clearLocalCache->Start(
        [&](ClearLocalCacheResult value) { result = std::move(value); }));

    QVERIFY(result.Succeeded());
    QVERIFY(result.removedCoverTemporaryFiles >= 1);
    QVERIFY(!QFileInfo::exists(temporaryPath));
}

void ApplicationCompositionTests::workerProductsReleaseConnectionsAndShutdownBeforeDatabase() {
    QTemporaryDir directory, root;
    for (int i = 0; i < 205; ++i) writeFile(root.filePath(QString::number(i) + ".mkv"));
    const auto before = QSqlDatabase::connectionNames();
    {
        auto context = createApplicationContext(optionsFor(directory));
        QVERIFY(context.localLibraryScan);
        // The adaptive sync runtime opens its long-lived connection asynchronously on its own worker;
        // wait for it so the snapshot isolates the scan worker's connection.
        QVERIFY(context.adaptiveSync);
        QTRY_VERIFY(context.adaptiveSync->isReady());
        const auto uiConnections = QSqlDatabase::connectionNames();
        context.userPreferences.libraryRoots = {root.path()};
        QObject lifetime;
        QSignalSpy started(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::started);
        QVERIFY(scheduleStartupLibraryScan(context, &lifetime));
        QTRY_COMPARE(started.count(), 1);
        context.localLibraryScan->shutdown();
        // A succeeded scan starts recognition on its own worker connection; stop it as main.cpp does.
        QVERIFY(context.localLibraryRecognition);
        context.localLibraryRecognition->shutdown();
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

void ApplicationCompositionTests::destroyedLifetimeCancelsScheduledStartup() {
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

void ApplicationCompositionTests::composesAdaptiveSynchronization() {
    QTemporaryDir directory;

    auto context = createApplicationContext(optionsFor(directory));

    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    QVERIFY(context.adaptiveSync);
}

QTEST_GUILESS_MAIN(ApplicationCompositionTests)
#include "ApplicationCompositionTests.moc"
