#include <QtTest>

#include <functional>
#include <vector>

#include "../../src/application/anilist/AniListAuthManager.h"
#include "../../src/application/anilist/RemoveAniListAccountUseCase.h"

namespace {
class RecordingSecretStore final : public ISecretStore {
public:
    explicit RecordingSecretStore(QStringList &events) : events_(events) {}
    bool loadAniListCredentials(AniListCredentials &credentials, QString &error) override {
        credentials = stored;
        error.clear();
        return true;
    }
    bool saveAniListCredentials(const AniListCredentials &credentials, QString &error) override {
        stored = credentials;
        error.clear();
        return true;
    }
    bool clearAniListCredentials(QString &error) override {
        events_.append(QStringLiteral("clear"));
        if (failClear) {
            error = QStringLiteral("store unavailable");
            return false;
        }
        stored = {};
        error.clear();
        return true;
    }

    AniListCredentials stored;
    bool failClear = false;

private:
    QStringList &events_;
};
}

class RemoveAniListAccountUseCaseTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void waitsForBothCancellationsBeforeClearingAndRemembersTheRemovedAccount();
    void rejectsASecondStartWhileRemovalIsRunning();
    void reportsFailureAndRemembersNothingWhenClearFails();
    void completesWhenCancellationsAcknowledgeImmediately();
    void logsWithoutCredentialDetails();

private:
    [[nodiscard]] RemoveAniListAccountUseCase MakeUseCase(bool deferAcknowledgements);

    QStringList events;
    RecordingSecretStore secretStore{events};
    AniListAuthManager authManager{secretStore};
    std::vector<std::function<void()>> pendingAcknowledgements;
    QStringList logs;
    qint64 remembered = 0;
};

void RemoveAniListAccountUseCaseTests::init() {
    events.clear();
    logs.clear();
    pendingAcknowledgements.clear();
    remembered = 0;
    secretStore.failClear = false;
    QString error;
    QVERIFY(authManager.save({.username = QStringLiteral("TestUser"), .token = QStringLiteral("secret-token"),
                              .userId = 42}, error));
}

RemoveAniListAccountUseCase RemoveAniListAccountUseCaseTests::MakeUseCase(const bool deferAcknowledgements) {
    return RemoveAniListAccountUseCase(
        [this] { events.append(QStringLiteral("stopListener")); },
        [this, deferAcknowledgements](const SyncPartition partition, std::function<void()> acknowledged) {
            events.append(QStringLiteral("cancel(%1)").arg(ToString(partition)));
            if (deferAcknowledgements) pendingAcknowledgements.push_back(std::move(acknowledged));
            else acknowledged();
        },
        authManager,
        [this](const qint64 userId, QString &error) {
            events.append(QStringLiteral("remember"));
            remembered = userId;
            error.clear();
            return true;
        },
        [this](const QString &message) { logs.append(message); });
}

void RemoveAniListAccountUseCaseTests::waitsForBothCancellationsBeforeClearingAndRemembersTheRemovedAccount() {
    auto useCase = MakeUseCase(true);
    bool completed = false;
    bool succeeded = false;

    QVERIFY(useCase.Start([&](const bool ok, const QString &) { completed = true; succeeded = ok; }));

    QCOMPARE(events, (QStringList{QStringLiteral("stopListener"), QStringLiteral("cancel(user-list)"),
                                   QStringLiteral("cancel(pending-changes)")}));
    QVERIFY(useCase.IsRunning());
    QCOMPARE(pendingAcknowledgements.size(), size_t{2});
    pendingAcknowledgements[1]();
    QVERIFY(!events.contains(QStringLiteral("clear")));
    pendingAcknowledgements[0]();

    QCOMPARE(events, (QStringList{QStringLiteral("stopListener"), QStringLiteral("cancel(user-list)"),
                                   QStringLiteral("cancel(pending-changes)"), QStringLiteral("clear"),
                                   QStringLiteral("remember")}));
    QVERIFY(completed);
    QVERIFY(succeeded);
    QCOMPARE(remembered, qint64{42});
    QVERIFY(!useCase.IsRunning());
    QVERIFY(authManager.credentials().token.isEmpty());
}

void RemoveAniListAccountUseCaseTests::rejectsASecondStartWhileRemovalIsRunning() {
    auto useCase = MakeUseCase(true);
    int completions = 0;

    QVERIFY(useCase.Start([&](bool, const QString &) { ++completions; }));
    QVERIFY(!useCase.Start([&](bool, const QString &) { ++completions; }));
    pendingAcknowledgements[0]();
    pendingAcknowledgements[1]();

    QCOMPARE(completions, 1);
    QCOMPARE(events.count(QStringLiteral("stopListener")), 1);
    QCOMPARE(events.count(QStringLiteral("clear")), 1);
}

void RemoveAniListAccountUseCaseTests::reportsFailureAndRemembersNothingWhenClearFails() {
    secretStore.failClear = true;
    auto useCase = MakeUseCase(false);
    bool succeeded = true;
    QString failure;

    QVERIFY(useCase.Start([&](const bool ok, const QString &error) { succeeded = ok; failure = error; }));

    QVERIFY(!succeeded);
    QVERIFY(!failure.isEmpty());
    QVERIFY(!events.contains(QStringLiteral("remember")));
    QCOMPARE(remembered, qint64{0});
    QVERIFY(!useCase.IsRunning());
    QCOMPARE(authManager.credentials().userId, qint64{42});
}

void RemoveAniListAccountUseCaseTests::completesWhenCancellationsAcknowledgeImmediately() {
    auto useCase = MakeUseCase(false);
    bool succeeded = false;

    QVERIFY(useCase.Start([&](const bool ok, const QString &) { succeeded = ok; }));

    QVERIFY(succeeded);
    QCOMPARE(remembered, qint64{42});
    QVERIFY(useCase.Start([](bool, const QString &) {}));
}

void RemoveAniListAccountUseCaseTests::logsWithoutCredentialDetails() {
    auto useCase = MakeUseCase(false);
    QVERIFY(useCase.Start([](bool, const QString &) {}));

    QVERIFY(logs.contains(QStringLiteral("AniList account removal started.")));
    QVERIFY(logs.contains(QStringLiteral("AniList account removal completed.")));
    for (const auto &line : std::as_const(logs)) {
        QVERIFY(!line.contains(QStringLiteral("secret-token")));
        QVERIFY(!line.contains(QStringLiteral("TestUser")));
    }
}

QTEST_MAIN(RemoveAniListAccountUseCaseTests)
#include "RemoveAniListAccountUseCaseTests.moc"
