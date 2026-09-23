#include <QtTest>

#include "../../src/application/anilist/AniListPendingChangeFactory.h"

class AniListPendingChangeFactoryTests : public QObject {
    Q_OBJECT

private slots:
    void createsChangeWithInitialState();
    void rejectsInvalidMediaId();
};

void AniListPendingChangeFactoryTests::createsChangeWithInitialState() {
    AniListPendingChange change;
    QString error;
    QVERIFY(AniListPendingChangeFactory::Create(
        154587, AniListField::Progress, 12, 13, change, error));
    QCOMPARE(change.mediaId, 154587);
    QCOMPARE(change.field, AniListField::Progress);
    QCOMPARE(std::get<int>(change.previousValue), 12);
    QCOMPARE(std::get<int>(change.newValue), 13);
    QCOMPARE(change.status, AniListPendingChangeStatus::Pending);
    QVERIFY(change.createdAt.isValid());
}

void AniListPendingChangeFactoryTests::rejectsInvalidMediaId() {
    AniListPendingChange change;
    QString error;
    QVERIFY(!AniListPendingChangeFactory::Create(
        0, AniListField::Progress, 12, 13, change, error));
    QVERIFY(!error.isEmpty());
}

QTEST_MAIN(AniListPendingChangeFactoryTests)
#include "AniListPendingChangeFactoryTests.moc"
