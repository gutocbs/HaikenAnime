#include <QtTest>

#include "../../src/application/anilist/AniListAuthManager.h"

class InMemorySecretStore final : public ISecretStore {
public:
    bool loadAniListCredentials(AniListCredentials &credentials, QString &error) override {
        if (!available) {
            error = QStringLiteral("credentials unavailable");
            return false;
        }

        credentials = storedCredentials;
        return true;
    }

    bool saveAniListCredentials(const AniListCredentials &credentials, QString &error) override {
        Q_UNUSED(error)
        storedCredentials = credentials;
        available = true;
        return true;
    }

    bool clearAniListCredentials(QString &error) override {
        error.clear();
        storedCredentials = {};
        available = false;
        return true;
    }

    AniListCredentials storedCredentials;
    bool available = false;
};

class AniListAuthManagerTests : public QObject {
    Q_OBJECT

private slots:
    void loadsCredentialsFromStore();
    void doesNotReplaceCacheWhenLoadFails();
    void savesCredentialsThroughStore();
    void savesAndLoadsSessionIdentity();
    void clearsStoredCredentialsAndCache();
};

void AniListAuthManagerTests::loadsCredentialsFromStore() {
    InMemorySecretStore store;
    store.storedCredentials = {QStringLiteral("user"), QStringLiteral("token")};
    store.available = true;
    AniListAuthManager manager(store);
    QString error;

    QVERIFY(manager.load(error));
    QCOMPARE(manager.credentials().username, QStringLiteral("user"));
    QCOMPARE(manager.credentials().token, QStringLiteral("token"));
}

void AniListAuthManagerTests::doesNotReplaceCacheWhenLoadFails() {
    InMemorySecretStore store;
    AniListAuthManager manager(store);
    QString error;
    const AniListCredentials expected{QStringLiteral("user"), QStringLiteral("token")};

    QVERIFY(manager.save(expected, error));
    store.available = false;
    QVERIFY(!manager.load(error));
    QCOMPARE(manager.credentials().username, expected.username);
    QCOMPARE(manager.credentials().token, expected.token);
}

void AniListAuthManagerTests::savesCredentialsThroughStore() {
    InMemorySecretStore store;
    AniListAuthManager manager(store);
    QString error;
    const AniListCredentials expected{QStringLiteral("user"), QStringLiteral("token")};

    QVERIFY(manager.save(expected, error));
    QCOMPARE(store.storedCredentials.username, expected.username);
    QCOMPARE(store.storedCredentials.token, expected.token);
}

void AniListAuthManagerTests::savesAndLoadsSessionIdentity() {
    InMemorySecretStore store;
    AniListAuthManager manager(store);
    QString error;
    AniListCredentials expected;
    expected.username = QStringLiteral("user");
    expected.token = QStringLiteral("token");
    expected.userId = 42;
    expected.expiresAtUnixSeconds = 1'800'000'000;

    QVERIFY(manager.save(expected, error));
    QVERIFY(manager.load(error));
    QCOMPARE(manager.credentials().userId, 42);
    QCOMPARE(manager.credentials().expiresAtUnixSeconds, qint64(1'800'000'000));
}

void AniListAuthManagerTests::clearsStoredCredentialsAndCache() {
    InMemorySecretStore store;
    AniListAuthManager manager(store);
    QString error;
    const AniListCredentials credentials{QStringLiteral("user"), QStringLiteral("token"), 42,
                                          1'800'000'000};

    QVERIFY(manager.save(credentials, error));
    QVERIFY(manager.clear(error));
    QVERIFY(manager.credentials().token.isEmpty());
    QVERIFY(!store.available);
}

QTEST_MAIN(AniListAuthManagerTests)
#include "AniListAuthManagerTests.moc"
