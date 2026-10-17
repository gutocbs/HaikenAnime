#include <QtTest>

#include "../../src/application/anilist/AniListAccountOwnership.h"

using namespace AniListAccountOwnership;

class AniListAccountOwnershipTests final : public QObject {
    Q_OBJECT

private slots:
    void remembersLoadedAccountWhenPreferenceIsUnset();
    void doesNotRememberWhenAlreadyStoredOrDisconnected();
    void ownerIsLoadedAccountAfterStartupStep();
    void ownerFallsBackToLastAccountWhenDisconnected();
    void pendingChangesAreSkippedWithoutValidatedAccount();
    void userListIsSkippedWithoutTokenOrAccount();
};

void AniListAccountOwnershipTests::remembersLoadedAccountWhenPreferenceIsUnset() {
    QCOMPARE(AccountToRemember(7, 0), qint64(7));
    QCOMPARE(AccountToRemember(7, 3), qint64(7));
}

void AniListAccountOwnershipTests::doesNotRememberWhenAlreadyStoredOrDisconnected() {
    QCOMPARE(AccountToRemember(7, 7), qint64(0));
    QCOMPARE(AccountToRemember(0, 7), qint64(0));
}

void AniListAccountOwnershipTests::ownerIsLoadedAccountAfterStartupStep() {
    qint64 last = 0;
    const qint64 connected = 7;
    if (const auto remembered = AccountToRemember(connected, last); remembered > 0) last = remembered;

    // The user disconnects and edits offline: the owner must still be the previous account.
    QCOMPARE(ResolveOwnerUserId(0, last), qint64(7));
}

void AniListAccountOwnershipTests::ownerFallsBackToLastAccountWhenDisconnected() {
    QCOMPARE(ResolveOwnerUserId(5, 7), qint64(5));
    QCOMPARE(ResolveOwnerUserId(0, 7), qint64(7));
    QCOMPARE(ResolveOwnerUserId(0, 0), qint64(0));
}

void AniListAccountOwnershipTests::pendingChangesAreSkippedWithoutValidatedAccount() {
    QVERIFY(!CanProcessPendingChanges(0));
    QVERIFY(CanProcessPendingChanges(7));
}

void AniListAccountOwnershipTests::userListIsSkippedWithoutTokenOrAccount() {
    AniListCredentials credentials;
    QVERIFY(!CanSynchronizeUserList(credentials));
    credentials.token = QStringLiteral("token");
    QVERIFY(!CanSynchronizeUserList(credentials));
    credentials.userId = 7;
    QVERIFY(CanSynchronizeUserList(credentials));
    credentials.token.clear();
    QVERIFY(!CanSynchronizeUserList(credentials));
}

QTEST_MAIN(AniListAccountOwnershipTests)
#include "AniListAccountOwnershipTests.moc"
