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

    AniListCredentials storedCredentials;
    bool available = false;
};

class AniListAuthManagerTests : public QObject {
    Q_OBJECT

private slots:
    void loadsCredentialsFromStore();
    void doesNotReplaceCacheWhenLoadFails();
    void savesCredentialsThroughStore();
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

QTEST_MAIN(AniListAuthManagerTests)
#include "AniListAuthManagerTests.moc"
