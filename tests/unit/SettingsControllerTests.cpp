#include <QSignalSpy>
#include <QtTest>
#include <limits>

#include "../../src/presentation/settings/SettingsController.h"

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

QTEST_MAIN(SettingsControllerTests)
#include "SettingsControllerTests.moc"
