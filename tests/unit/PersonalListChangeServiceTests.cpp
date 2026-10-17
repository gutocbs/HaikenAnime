#include <QtTest>

#include "../../src/application/media/PersonalListChangeService.h"

class RecordingPersonalListChangeWriter final : public IPersonalListChangeWriter {
public:
    bool remove(const Media &media, const AniListPendingChange &deletion, QString &error) override {
        removedMedia = media;
        removedDeletion = deletion;
        ++removeCalls;
        error = failure;
        return failure.isEmpty();
    }

    Media removedMedia;
    AniListPendingChange removedDeletion;
    int removeCalls = 0;
    bool save(const Media &media, const QList<AniListPendingChange> &changes,
              QString &error) override {
        savedMedia = media;
        savedChanges = changes;
        ++calls;
        error = failure;
        return failure.isEmpty();
    }

    Media savedMedia;
    QList<AniListPendingChange> savedChanges;
    QString failure;
    int calls = 0;
};

class PersonalListChangeServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void persistsLocalEditWithRemotePendingChanges();
    void requestsPendingSynchronizationOnlyAfterDurableOutboxWrite();
    void stampsPendingChangesWithOwnerFromProvider();
    void leavesOwnerUnsetWhenNoProviderIsConfigured();
    void queuesConfirmedDeletionAsPendingStampedWithOwner();
    void notifiesPendingChangesOnlyAfterRemovalIsDurable();
    void rejectsRemovalOfMediaWithoutValidIdentifier();
};

void PersonalListChangeServiceTests::persistsLocalEditWithRemotePendingChanges() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    Media previous;
    previous.Id = 154587;
    previous.ConsumedChapters = 4;
    previous.PersonalScore = 7;
    previous.ListStatus = UserListStatus::Current;
    Media updated = previous;
    updated.ConsumedChapters = 12;
    updated.PersonalScore = 9;
    updated.ListStatus = UserListStatus::Completed;
    QString error;

    QVERIFY2(service.save(previous, updated, error), qPrintable(error));

    QCOMPARE(writer.calls, 1);
    QCOMPARE(writer.savedMedia.Id, 154587);
    QCOMPARE(writer.savedMedia.ConsumedChapters, 12);
    QCOMPARE(writer.savedChanges.size(), 3);
    QCOMPARE(writer.savedChanges.at(0).field, AniListField::Progress);
    QCOMPARE(writer.savedChanges.at(0).previousValue, AniListFieldValue(4));
    QCOMPARE(writer.savedChanges.at(0).newValue, AniListFieldValue(12));
    QCOMPARE(writer.savedChanges.at(1).field, AniListField::PersonalScore);
    QCOMPARE(writer.savedChanges.at(2).field, AniListField::ListStatus);
}

void PersonalListChangeServiceTests::requestsPendingSynchronizationOnlyAfterDurableOutboxWrite() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    int requests = 0;
    service.setPendingChangesNotifier([&requests] { ++requests; });
    Media previous;
    previous.Id = 154587;
    previous.ConsumedChapters = 4;
    Media updated = previous;
    updated.ConsumedChapters = 5;
    QString error;

    writer.failure = QStringLiteral("database write failed");
    QVERIFY(!service.save(previous, updated, error));
    QCOMPARE(requests, 0);

    writer.failure.clear();
    QVERIFY2(service.save(previous, updated, error), qPrintable(error));
    QCOMPARE(requests, 1);
}

void PersonalListChangeServiceTests::stampsPendingChangesWithOwnerFromProvider() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    service.setOwnerProvider([] { return qint64(77); });
    Media previous;
    previous.Id = 1;
    previous.ConsumedChapters = 1;
    previous.PersonalScore = 5;
    Media updated = previous;
    updated.ConsumedChapters = 2;
    updated.PersonalScore = 6;
    QString error;

    QVERIFY2(service.save(previous, updated, error), qPrintable(error));

    QCOMPARE(writer.savedChanges.size(), 2);
    QCOMPARE(writer.savedChanges.at(0).ownerUserId, qint64(77));
    QCOMPARE(writer.savedChanges.at(1).ownerUserId, qint64(77));
}

void PersonalListChangeServiceTests::leavesOwnerUnsetWhenNoProviderIsConfigured() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    Media previous;
    previous.Id = 1;
    Media updated = previous;
    updated.ConsumedChapters = 2;
    QString error;

    QVERIFY2(service.save(previous, updated, error), qPrintable(error));

    QCOMPARE(writer.savedChanges.size(), 1);
    QCOMPARE(writer.savedChanges.first().ownerUserId, qint64(0));
}

void PersonalListChangeServiceTests::queuesConfirmedDeletionAsPendingStampedWithOwner() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    service.setOwnerProvider([] { return qint64(77); });
    Media media;
    media.Id = 42;
    media.ListStatus = UserListStatus::Current;
    QString error;

    QVERIFY2(service.remove(media, error), qPrintable(error));

    QCOMPARE(writer.removeCalls, 1);
    QCOMPARE(writer.removedMedia.Id, 42);
    QCOMPARE(writer.removedDeletion.mediaId, 42);
    QCOMPARE(writer.removedDeletion.field, AniListField::Deletion);
    QCOMPARE(writer.removedDeletion.status, AniListPendingChangeStatus::Pending);
    QCOMPARE(writer.removedDeletion.ownerUserId, qint64(77));
}

void PersonalListChangeServiceTests::notifiesPendingChangesOnlyAfterRemovalIsDurable() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    int requests = 0;
    service.setPendingChangesNotifier([&requests] { ++requests; });
    Media media;
    media.Id = 42;
    QString error;

    writer.failure = QStringLiteral("database write failed");
    QVERIFY(!service.remove(media, error));
    QCOMPARE(error, writer.failure);
    QCOMPARE(requests, 0);

    writer.failure.clear();
    QVERIFY2(service.remove(media, error), qPrintable(error));
    QCOMPARE(requests, 1);
}

void PersonalListChangeServiceTests::rejectsRemovalOfMediaWithoutValidIdentifier() {
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService service(writer);
    Media media;
    QString error;

    QVERIFY(!service.remove(media, error));

    QVERIFY(!error.isEmpty());
    QCOMPARE(writer.removeCalls, 0);
}

QTEST_MAIN(PersonalListChangeServiceTests)
#include "PersonalListChangeServiceTests.moc"
