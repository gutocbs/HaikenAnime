#include <QtTest>

#include "../../src/application/anilist/AniListSyncErrorClassifier.h"

class AniListSyncErrorClassifierTests : public QObject {
    Q_OBJECT

private slots:
    void classifiesAuthenticationAsPermanent();
    void classifiesNetworkAsRetryable();
    void classifiesTimeoutAsRetryable();
    void classifiesPersistenceAsPermanent();
};

void AniListSyncErrorClassifierTests::classifiesAuthenticationAsPermanent() {
    const auto category = AniListSyncErrorClassifier::Classify(QStringLiteral("Invalid credentials"));
    QCOMPARE(category, AniListSyncErrorCategory::Authentication);
    QVERIFY(!IsRetryable(category));
}

void AniListSyncErrorClassifierTests::classifiesNetworkAsRetryable() {
    const auto category = AniListSyncErrorClassifier::Classify(QStringLiteral("Connection refused"));
    QCOMPARE(category, AniListSyncErrorCategory::Network);
    QVERIFY(IsRetryable(category));
}

void AniListSyncErrorClassifierTests::classifiesTimeoutAsRetryable() {
    const auto category = AniListSyncErrorClassifier::Classify(QStringLiteral("Synchronization timed out"));
    QCOMPARE(category, AniListSyncErrorCategory::Timeout);
    QVERIFY(IsRetryable(category));
}

void AniListSyncErrorClassifierTests::classifiesPersistenceAsPermanent() {
    const auto category = AniListSyncErrorClassifier::Classify(QStringLiteral("SQLite write failed"));
    QCOMPARE(category, AniListSyncErrorCategory::Persistence);
    QVERIFY(!IsRetryable(category));
}

QTEST_MAIN(AniListSyncErrorClassifierTests)
#include "AniListSyncErrorClassifierTests.moc"
