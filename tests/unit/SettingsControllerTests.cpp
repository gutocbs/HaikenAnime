#include <QSignalSpy>
#include <QtTest>
#include <limits>
#include <QMutexLocker>
#include <QThread>
#include <atomic>

#include "../../src/presentation/settings/SettingsController.h"
#include "../../src/app/LocalLibraryScanCoordinator.h"

struct ScanFixture {
    QMutex mutex;
    LocalLibraryScanRequest request;
    std::atomic_bool hold = false;
};

class ControlledScanner final : public ILocalLibraryScanner {
public:
    explicit ControlledScanner(std::shared_ptr<ScanFixture> fixture) : fixture_(std::move(fixture)) {}
    LocalLibraryScanResult scan(const LocalLibraryScanRequest &request,
        const std::function<bool(const QList<LocalFileObservation> &, QString &)> &,
        const std::function<void(const LocalLibraryScanProgress &)> &,
        const std::function<bool()> &stop) override {
        { QMutexLocker lock(&fixture_->mutex); fixture_->request = request; }
        while (fixture_->hold && !stop()) QThread::msleep(1);
        return {!stop(), stop(), 7, {}};
    }
private:
    std::shared_ptr<ScanFixture> fixture_;
};

class ScanRepository final : public ILocalFileRepository {
public:
    bool beginScan(const QString &, qint64 &id, QString &) override { id = 1; return true; }
    bool upsertBatch(qint64, const QList<LocalFileObservation> &, QString &) override { return true; }
    bool completeScan(qint64, qsizetype, QString &) override { return true; }
    bool failScan(qint64, LibraryScanStatus, qsizetype, const QString &, QString &) override { return true; }
};

static LocalLibraryScanCoordinator::ScannerFactory scannerFactory(const std::shared_ptr<ScanFixture> &fixture) {
    return [fixture](QString &) { return std::make_unique<ControlledScanner>(fixture); };
}

static LocalLibraryScanCoordinator::RepositoryFactory scanRepositoryFactory() {
    return [](QString &) { return std::make_unique<ScanRepository>(); };
}

class FakePreferencesRepository final : public IUserPreferencesRepository {
public:
    bool read(UserPreferences &, bool &, QString &) override { return false; }
    bool replace(const UserPreferences &value, QString &error) override {
        ++replaceCalls;
        if (fail) { error = QStringLiteral("write failed"); return false; }
        stored = value;
        return true;
    }
    int replaceCalls = 0;
    bool fail = false;
    UserPreferences stored;
};

class SettingsControllerTests final : public QObject {
    Q_OBJECT
private slots:
    void editsSavesAndEmitsCompletePreferences();
    void invalidDraftDoesNotCallRepository();
    void repositoryFailurePreservesDraftAndDoesNotApply();
    void discardRestoresPersistedSnapshot();
    void appliesExternalHomeSortToBothSnapshots();
    void savesBackendProvidedCardStatusPresentation();
    void rejectsInvalidCardStatusPresentation();
    void exposesBackendLanguageOptionsAndRestartNotice();
    void exposesBackendPreferredTitleOptionsAndRestartNotice();
    void libraryDraftSavesAndDiscardsAtomically();
    void invalidLibraryDraftCannotSave();
    void extensionsAreNormalizedAndOptionsRetainPersistedSelections();
    void scanRejectsDirtyAndUnavailableCoordinator();
    void scanUsesPersistedRequestAndRejectsConcurrentAction();
    void scanSignalsUpdatePresentation();
    void coordinatorReplacementDisconnectsOldSignals();
};

void SettingsControllerTests::editsSavesAndEmitsCompletePreferences() {
    FakePreferencesRepository repository;
    UserPreferences initial;
    SettingsController controller(&repository, initial);
    QSignalSpy applied(&controller, &SettingsController::preferencesApplied);

    controller.SetScoreScale(0, 100, 5);
    controller.SetCoverQuality(QStringLiteral("large"));
    controller.SetSynchronizationEnabled(false);
    controller.SetSynchronizationInterval(1800000);
    QVERIFY(controller.dirty());
    QVERIFY(controller.valid());
    controller.Save();

    QCOMPARE(repository.replaceCalls, 1);
    QCOMPARE(repository.stored.scoreMaximum, 100.0);
    QCOMPARE(repository.stored.coverQuality, CoverQuality::Large);
    QVERIFY(!repository.stored.synchronizationEnabled);
    QCOMPARE(applied.count(), 1);
    QVERIFY(!controller.dirty());
    QVERIFY(controller.errorMessage().isEmpty());
}

void SettingsControllerTests::invalidDraftDoesNotCallRepository() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});
    controller.SetScoreScale(std::numeric_limits<double>::quiet_NaN(), 10, 1);
    QVERIFY(!controller.valid());
    controller.Save();
    QCOMPARE(repository.replaceCalls, 0);
    QVERIFY(!controller.errorMessage().isEmpty());
}

void SettingsControllerTests::repositoryFailurePreservesDraftAndDoesNotApply() {
    FakePreferencesRepository repository;
    repository.fail = true;
    SettingsController controller(&repository, {});
    QSignalSpy applied(&controller, &SettingsController::preferencesApplied);
    controller.SetCoverQuality(QStringLiteral("extraLarge"));
    controller.Save();
    QCOMPARE(repository.replaceCalls, 1);
    QCOMPARE(controller.coverQualityKey(), QStringLiteral("extraLarge"));
    QVERIFY(controller.dirty());
    QVERIFY(!controller.errorMessage().isEmpty());
    QCOMPARE(applied.count(), 0);
}

void SettingsControllerTests::discardRestoresPersistedSnapshot() {
    FakePreferencesRepository repository;
    UserPreferences initial;
    initial.scoreMaximum = 100;
    initial.scoreStep = 5;
    SettingsController controller(&repository, initial);
    controller.SetScoreScale(0, 10, 1);
    controller.Discard();
    QCOMPARE(controller.scoreMaximum(), 100.0);
    QCOMPARE(controller.scoreStep(), 5.0);
    QVERIFY(!controller.dirty());
    QVERIFY(controller.synchronizationIntervalOptions().size() == 7);
    QVERIFY(controller.coverQualityOptions().size() == 3);
    QVERIFY(controller.scoreScaleOptions().size() == 3);
}

void SettingsControllerTests::libraryDraftSavesAndDiscardsAtomically() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});
    controller.SetLibraryRoot(QStringLiteral("D:\\Anime"));
    controller.SetScanExtensionEnabled(QStringLiteral(".avi"), false);
    QVERIFY(controller.dirty());
    QVERIFY(controller.valid());
    controller.Discard();
    QCOMPARE(controller.libraryRoot(), QStringLiteral("Q:\\"));
    QVERIFY(controller.selectedScanExtensions().contains(QStringLiteral(".avi")));
    QVERIFY(!controller.dirty());
    controller.SetLibraryRoot(QStringLiteral("D:\\Anime"));
    controller.SetScanExtensionEnabled(QStringLiteral(".avi"), false);
    repository.fail = true;
    controller.Save();
    QVERIFY(controller.dirty());
    QCOMPARE(controller.libraryRoot(), QStringLiteral("D:\\Anime"));
    repository.fail = false;
    controller.Save();
    QCOMPARE(repository.stored.libraryRoot, QStringLiteral("D:\\Anime"));
    QVERIFY(!repository.stored.scanExtensions.contains(QStringLiteral(".avi")));
    QVERIFY(!controller.dirty());
}

void SettingsControllerTests::appliesExternalHomeSortToBothSnapshots() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});

    controller.ApplyExternalHomeSortKey(QStringLiteral("title_desc"));

    QVERIFY(!controller.dirty());
    controller.SetCoverQuality(QStringLiteral("large"));
    controller.Save();
    QCOMPARE(repository.stored.homeSortKey, QStringLiteral("title_desc"));
}

void SettingsControllerTests::savesBackendProvidedCardStatusPresentation() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});

    QCOMPARE(controller.cardStatusPresentationOptions().size(), 2);
    QCOMPARE(controller.cardStatusPresentationKey(), QStringLiteral("personal-list-status"));
    controller.SetCardStatusPresentation(QStringLiteral("media-release-status"));
    QVERIFY(controller.valid());
    controller.Save();

    QCOMPARE(repository.stored.cardStatusPresentation,
             CardStatusPresentation::MediaReleaseStatus);
}

void SettingsControllerTests::rejectsInvalidCardStatusPresentation() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});

    controller.SetCardStatusPresentation(QStringLiteral("unsupported"));

    QVERIFY(!controller.valid());
    controller.Save();
    QCOMPARE(repository.replaceCalls, 0);
    controller.Discard();
    QCOMPARE(controller.cardStatusPresentationKey(), QStringLiteral("personal-list-status"));
}

void SettingsControllerTests::exposesBackendLanguageOptionsAndRestartNotice() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});

    QCOMPARE(controller.languageOptions().size(), 2);
    QCOMPARE(controller.languageKey(), QStringLiteral("pt-BR"));
    QVERIFY(controller.restartRequiredMessage().isEmpty());

    controller.SetLanguage(QStringLiteral("en"));
    QVERIFY(controller.valid());
    QVERIFY(!controller.restartRequiredMessage().isEmpty());
    controller.Save();
    QCOMPARE(repository.stored.languageKey, QStringLiteral("en"));
    QVERIFY(!controller.restartRequiredMessage().isEmpty());

    controller.SetLanguage(QStringLiteral("obsolete"));
    QVERIFY(!controller.valid());
    controller.Discard();
    QCOMPARE(controller.languageKey(), QStringLiteral("en"));
}

void SettingsControllerTests::exposesBackendPreferredTitleOptionsAndRestartNotice() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});

    QCOMPARE(controller.preferredTitleOptions().size(), 3);
    QCOMPARE(controller.preferredTitleKey(), QStringLiteral("romaji"));
    QVERIFY(controller.restartRequiredMessage().isEmpty());

    controller.SetPreferredTitle(QStringLiteral("native"));
    QVERIFY(controller.valid());
    QVERIFY(!controller.restartRequiredMessage().isEmpty());
    controller.Save();
    QCOMPARE(repository.stored.preferredTitleKey, QStringLiteral("native"));
    QVERIFY(!controller.restartRequiredMessage().isEmpty());

    controller.SetPreferredTitle(QStringLiteral("obsolete"));
    QVERIFY(!controller.valid());
    controller.Discard();
    QCOMPARE(controller.preferredTitleKey(), QStringLiteral("native"));
}

void SettingsControllerTests::invalidLibraryDraftCannotSave() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});
    controller.SetLibraryRoot(QStringLiteral("   "));
    QVERIFY(!controller.valid());
    controller.Save();
    QCOMPARE(repository.replaceCalls, 0);
    controller.Discard();
    for (const auto &extension : controller.selectedScanExtensions())
        controller.SetScanExtensionEnabled(extension, false);
    QVERIFY(!controller.valid());
    QVERIFY(!controller.errorMessage().isEmpty());
    controller.Save();
    QCOMPARE(repository.replaceCalls, 0);
    controller.Discard();
    controller.SetScanExtensionEnabled(QStringLiteral("folder/mp4"), true);
    QVERIFY(!controller.valid());
    controller.Save();
    QCOMPARE(repository.replaceCalls, 0);
    controller.Discard();
    QVERIFY(controller.valid());
    QVERIFY(controller.errorMessage().isEmpty());
}

void SettingsControllerTests::extensionsAreNormalizedAndOptionsRetainPersistedSelections() {
    FakePreferencesRepository repository;
    UserPreferences initial;
    initial.scanExtensions = {QStringLiteral(".mkv"), QStringLiteral(".custom")};
    SettingsController controller(&repository, initial);
    QVERIFY(controller.availableScanExtensions().contains(QStringLiteral(".mp4")));
    QVERIFY(controller.availableScanExtensions().contains(QStringLiteral(".custom")));
    controller.SetScanExtensionEnabled(QStringLiteral("MP4"), true);
    controller.SetScanExtensionEnabled(QStringLiteral("..MP4"), true);
    QCOMPARE(controller.selectedScanExtensions(), QStringList({QStringLiteral(".mkv"), QStringLiteral(".custom"), QStringLiteral(".mp4")}));
    controller.Save();
    QVERIFY(controller.valid());
    QCOMPARE(repository.stored.scanExtensions.last(), QStringLiteral(".mp4"));
    controller.SetScanExtensionEnabled(QStringLiteral("MKV"), false);
    QVERIFY(!controller.selectedScanExtensions().contains(QStringLiteral(".mkv")));
}

void SettingsControllerTests::scanRejectsDirtyAndUnavailableCoordinator() {
    FakePreferencesRepository repository;
    SettingsController controller(&repository, {});
    controller.ScanNow();
    QVERIFY(!controller.scanRunning());
    QVERIFY(!controller.scanErrorMessage().isEmpty());
    auto fixture = std::make_shared<ScanFixture>();
    LocalLibraryScanCoordinator coordinator(scannerFactory(fixture), scanRepositoryFactory());
    controller.SetScanCoordinator(&coordinator);
    QSignalSpy started(&coordinator, &LocalLibraryScanCoordinator::started);
    controller.SetLibraryRoot(QStringLiteral("D:\\Draft"));
    controller.ScanNow();
    QVERIFY(!controller.scanRunning());
    QVERIFY(!controller.scanErrorMessage().isEmpty());
    QTest::qWait(20);
    QCOMPARE(started.count(), 0);
}

void SettingsControllerTests::scanUsesPersistedRequestAndRejectsConcurrentAction() {
    FakePreferencesRepository repository;
    UserPreferences initial;
    initial.libraryRoot = QStringLiteral("D:\\Saved");
    initial.scanExtensions = {QStringLiteral(".mp4")};
    auto fixture = std::make_shared<ScanFixture>();
    fixture->hold = true;
    LocalLibraryScanCoordinator coordinator(scannerFactory(fixture), scanRepositoryFactory());
    SettingsController controller(&repository, initial);
    controller.SetScanCoordinator(&coordinator);
    QSignalSpy started(&coordinator, &LocalLibraryScanCoordinator::started);
    controller.ScanNow();
    QVERIFY(controller.scanRunning());
    controller.ScanNow();
    QTRY_COMPARE(started.count(), 1);
    controller.ScanNow();
    QVERIFY(!controller.scanErrorMessage().isEmpty());
    controller.SetLibraryRoot(QStringLiteral("D:\\Unsaved"));
    fixture->hold = false;
    QTRY_VERIFY(!controller.scanRunning());
    { QMutexLocker lock(&fixture->mutex);
      QCOMPARE(fixture->request.rootPath, QStringLiteral("D:\\Saved"));
      QCOMPARE(fixture->request.allowedExtensions, QStringList({QStringLiteral(".mp4")}));
      QCOMPARE(fixture->request.batchSize, 200);
      QCOMPARE(fixture->request.batchPauseMs, 25); }
    QCOMPARE(controller.scanCandidateCount(), 7);
    QVERIFY(controller.scanErrorMessage().isEmpty());
    controller.Discard();
    controller.SetLibraryRoot(QStringLiteral("E:\\NewSaved"));
    controller.Save();
    controller.ScanNow();
    QTRY_COMPARE(started.count(), 2);
    QTRY_VERIFY(!controller.scanRunning());
    { QMutexLocker lock(&fixture->mutex);
      QCOMPARE(fixture->request.rootPath, QStringLiteral("E:\\NewSaved")); }
}

void SettingsControllerTests::scanSignalsUpdatePresentation() {
    SettingsController controller(nullptr, {});
    LocalLibraryScanCoordinator coordinator({}, {});
    controller.SetScanCoordinator(&coordinator);
    QVERIFY(!controller.scanStatusMessage().isEmpty());
    emit coordinator.started(QStringLiteral("D:\\Active"));
    QVERIFY(controller.scanRunning());
    QVERIFY(controller.scanStatusMessage().contains(QStringLiteral("D:\\Active")));
    QCOMPARE(controller.scanCandidateCount(), 0);
    emit coordinator.progressChanged(12);
    QCOMPARE(controller.scanCandidateCount(), 12);
    emit coordinator.failed(QStringLiteral("Cannot read child"));
    QVERIFY(!controller.scanRunning());
    QCOMPARE(controller.scanErrorMessage(), QStringLiteral("Cannot read child"));
    QCOMPARE(controller.scanCandidateCount(), 12);
    emit coordinator.started(QStringLiteral("E:\\Again"));
    QVERIFY(controller.scanErrorMessage().isEmpty());
    QCOMPARE(controller.scanCandidateCount(), 0);
    emit coordinator.completed(20);
    QVERIFY(!controller.scanRunning());
    QCOMPARE(controller.scanCandidateCount(), 20);
    QVERIFY(controller.scanErrorMessage().isEmpty());
    QVERIFY(!controller.scanStatusMessage().isEmpty());
}

void SettingsControllerTests::coordinatorReplacementDisconnectsOldSignals() {
    SettingsController controller(nullptr, {});
    LocalLibraryScanCoordinator first({}, {});
    auto second = std::make_unique<LocalLibraryScanCoordinator>(LocalLibraryScanCoordinator::ScannerFactory{}, LocalLibraryScanCoordinator::RepositoryFactory{});
    controller.SetScanCoordinator(&first);
    controller.SetScanCoordinator(second.get());
    emit first.started(QStringLiteral("old"));
    QVERIFY(!controller.scanRunning());
    emit second->started(QStringLiteral("current"));
    QVERIFY(controller.scanRunning());
    controller.SetScanCoordinator(nullptr);
    QVERIFY(!controller.scanRunning());
    emit second->started(QStringLiteral("detached"));
    QVERIFY(!controller.scanRunning());
    controller.SetScanCoordinator(second.get());
    emit second->started(QStringLiteral("current"));
    second.reset();
    QVERIFY(!controller.scanRunning());
    controller.ScanNow();
    QVERIFY(!controller.scanErrorMessage().isEmpty());
}

QTEST_MAIN(SettingsControllerTests)
#include "SettingsControllerTests.moc"
