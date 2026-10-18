#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/app/ApplicationComposition.h"

namespace {
ApplicationCompositionOptions optionsFor(const QTemporaryDir &directory) {
    ApplicationCompositionOptions options;
    options.databasePath = directory.filePath(QStringLiteral("library.sqlite"));
    return options;
}

void writeFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("fixture") != 7) qFatal("Cannot write fixture");
}
}

class LocalLibraryRecognitionCompositionTests final : public QObject {
    Q_OBJECT

private slots:
    void composesOneRecognitionCoordinatorWithTheApplicationContext();
    void successfulStartupScanStartsRecognitionForTheScannedRoot();
    void failedStartupScanDoesNotStartRecognition();
    void scanRecognitionPersistsAssociationAndMakesNextEpisodeAvailable();
};

void LocalLibraryRecognitionCompositionTests::composesOneRecognitionCoordinatorWithTheApplicationContext() {
    QTemporaryDir directory;

    auto context = createApplicationContext(optionsFor(directory));

    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    QVERIFY(context.localLibraryScan);
    QVERIFY(context.localLibraryRecognition);
    QVERIFY(context.localEpisodeReader);
    QVERIFY(context.localFileOpener);
}

void LocalLibraryRecognitionCompositionTests::successfulStartupScanStartsRecognitionForTheScannedRoot() {
    QTemporaryDir directory;
    QTemporaryDir root;
    writeFile(root.filePath(QStringLiteral("[Group] Example - 01.mkv")));
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    context.userPreferences.libraryRoots = {root.path()};
    context.userPreferences.scanExtensions = {QStringLiteral(".mkv")};
    QObject lifetime;
    QSignalSpy recognitionCompleted(context.localLibraryRecognition.get(),
                                    &LocalLibraryRecognitionCoordinator::completed);

    QVERIFY(scheduleStartupLibraryScan(context, &lifetime));

    QTRY_COMPARE(recognitionCompleted.count(), 1);
}

void LocalLibraryRecognitionCompositionTests::failedStartupScanDoesNotStartRecognition() {
    QTemporaryDir directory;
    QTemporaryDir root;
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    // A scan request without extensions is rejected by the scanner, so the startup scan fails.
    context.userPreferences.libraryRoots = {root.path()};
    context.userPreferences.scanExtensions = {};
    QObject lifetime;
    QSignalSpy scanFailed(context.localLibraryScan.get(), &LocalLibraryScanCoordinator::failed);
    QSignalSpy recognitionCompleted(context.localLibraryRecognition.get(),
                                    &LocalLibraryRecognitionCoordinator::completed);

    QVERIFY(scheduleStartupLibraryScan(context, &lifetime));
    QTRY_COMPARE(scanFailed.count(), 1);
    QTest::qWait(50);
    QCOMPARE(recognitionCompleted.count(), 0);
}

void LocalLibraryRecognitionCompositionTests::scanRecognitionPersistsAssociationAndMakesNextEpisodeAvailable() {
    QTemporaryDir directory;
    QTemporaryDir root;
    QVERIFY(QDir(root.path()).mkpath(QStringLiteral("Copy")));
    writeFile(root.filePath(QStringLiteral("[Group] Example - 01.mkv")));
    writeFile(root.filePath(QStringLiteral("Copy/[Group] Example - 01.mkv")));
    auto context = createApplicationContext(optionsFor(directory));
    QVERIFY2(context.isReady(), qPrintable(context.initializationError));
    QSqlQuery media(context.database->connection());
    QVERIFY(media.exec(QStringLiteral(
        "INSERT INTO media (id, name, alternative_names, type, status) "
        "VALUES (42, 'Example', '[]', 0, 0)")));
    context.userPreferences.libraryRoots = {root.path()};
    context.userPreferences.scanExtensions = {QStringLiteral(".mkv")};
    QObject lifetime;
    QSignalSpy recognitionCompleted(context.localLibraryRecognition.get(),
                                    &LocalLibraryRecognitionCoordinator::completed);
    QSignalSpy recognitionFailed(context.localLibraryRecognition.get(),
                                 &LocalLibraryRecognitionCoordinator::failed);
    QSignalSpy recognitionBatchPersisted(context.localLibraryRecognition.get(), SIGNAL(batchPersisted()));
    QVERIFY(recognitionBatchPersisted.isValid());

    QVERIFY(scheduleStartupLibraryScan(context, &lifetime));
    QTRY_VERIFY(recognitionCompleted.count() + recognitionFailed.count() == 1);
    QVERIFY2(recognitionCompleted.count() == 1,
             recognitionFailed.isEmpty() ? "Recognition did not complete."
                                         : qPrintable(recognitionFailed.first().first().toString()));
    QVERIFY(recognitionBatchPersisted.count() >= 1);

    QSqlQuery files(context.database->connection());
    QVERIFY(files.exec(QStringLiteral(
        "SELECT recognition_state, media_id, episode FROM local_files ORDER BY normalized_relative_path")));
    int associatedFiles = 0;
    while (files.next()) {
        QCOMPARE(files.value(0).toString(), QStringLiteral("associated"));
        QCOMPARE(files.value(1).toInt(), 42);
        QCOMPARE(files.value(2).toInt(), 1);
        ++associatedFiles;
    }
    QCOMPARE(associatedFiles, 2);
    LocalEpisode next;
    QString error;
    QVERIFY2(context.localEpisodeReader->readNextEpisode(42, 0, next, error), qPrintable(error));
    QCOMPARE(next.episode, 1);
    QVERIFY(QFileInfo::exists(next.path));
}

QTEST_GUILESS_MAIN(LocalLibraryRecognitionCompositionTests)
#include "LocalLibraryRecognitionCompositionTests.moc"
