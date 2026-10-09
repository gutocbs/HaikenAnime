#include <QtTest>

#include "../../src/infrastructure/secrets/WindowsCredentialStore.h"

class WindowsCredentialStoreTests : public QObject {
    Q_OBJECT

private slots:
    void savesLoadsAndClearsCredentials();
    void rejectsEmptyCredentialsBeforeCallingWindows();
};

void WindowsCredentialStoreTests::savesLoadsAndClearsCredentials() {
    const QString target = QStringLiteral("HaikenAnime.Tests.%1")
                               .arg(QCoreApplication::applicationPid());
    WindowsCredentialStore store(target);
    AniListCredentials expected;
    expected.username = QStringLiteral("test-user");
    expected.token = QStringLiteral("test-token");
    expected.userId = 42;
    expected.expiresAtUnixSeconds = 1'800'000'000;
    QString error;

    if (!store.saveAniListCredentials(expected, error)) {
        QSKIP(qPrintable(error));
    }

    AniListCredentials loaded;
    QVERIFY2(store.loadAniListCredentials(loaded, error), qPrintable(error));
    QCOMPARE(loaded.username, expected.username);
    QCOMPARE(loaded.token, expected.token);
    QCOMPARE(loaded.userId, expected.userId);
    QCOMPARE(loaded.expiresAtUnixSeconds, expected.expiresAtUnixSeconds);

    QVERIFY2(store.clearAniListCredentials(error), qPrintable(error));
}

void WindowsCredentialStoreTests::rejectsEmptyCredentialsBeforeCallingWindows() {
    WindowsCredentialStore store(QStringLiteral("HaikenAnime.Tests.Invalid"));
    QString error;

    QVERIFY(!store.saveAniListCredentials({}, error));
    QCOMPARE(error, QStringLiteral("AniList credentials must contain non-empty single-line username and token values."));
}

QTEST_MAIN(WindowsCredentialStoreTests)
#include "WindowsCredentialStoreTests.moc"
