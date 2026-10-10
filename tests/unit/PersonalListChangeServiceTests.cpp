#include <QtTest>

#include "../../src/application/media/PersonalListChangeService.h"

class RecordingPersonalListChangeWriter final : public IPersonalListChangeWriter {
public:
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

QTEST_MAIN(PersonalListChangeServiceTests)
#include "PersonalListChangeServiceTests.moc"
