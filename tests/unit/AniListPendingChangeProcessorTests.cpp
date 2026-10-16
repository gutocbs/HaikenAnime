#include <QtTest>

#include "../../src/application/anilist/AniListPendingChangeProcessor.h"
#include "../../src/application/anilist/AniListPendingChangeCompactor.h"
#include "../../src/application/anilist/AniListPendingChangeGrouper.h"

class FakePendingRepository final : public IPendingChangeRepository {
public:
    bool enqueue(const AniListPendingChange &change, QString &error) override {
        Q_UNUSED(error)
        changes.append(change);
        return true;
    }

    bool getPending(int mediaId, QList<AniListPendingChange> &result, QString &error) override {
        Q_UNUSED(error)
        for (const auto &change : changes) {
            if (change.mediaId == mediaId && change.status != AniListPendingChangeStatus::Succeeded) {
                result.append(change);
            }
        }
        return true;
    }

    bool getMediaIdsWithUnsentChanges(QSet<int> &result, QString &error) override {
        result.clear();
        error.clear();
        return true;
    }
    bool getPendingMediaIds(QList<int> &result, QString &error) override {
        result.clear();
        error.clear();
        return true;
    }

    bool updateStatus(const AniListPendingChange &change, QString &error) override {
        Q_UNUSED(error)
        statuses.append(change.status);
        updates.append(change);
        return true;
    }

    QList<AniListPendingChange> changes;
    QList<AniListPendingChangeStatus> statuses;
    QList<AniListPendingChange> updates;
};

class FakeUpdateClient final : public IAniListUpdateClient {
public:
    bool updateMedia(const AniListMediaPendingChanges &changes, QString &error) override {
        mediaCalls++;
        lastGroup = changes;
        for (const auto &change : changes.changes) {
            if (change.field == AniListField::Deletion &&
                change.status != AniListPendingChangeStatus::RequiresConfirmation) {
                error = QStringLiteral("Deletion requires confirmation.");
                return false;
            }
        }
        if (!succeeds) {
            error = QStringLiteral("update failed");
        }
        return succeeds;
    }

    bool deleteMediaListEntry(int mediaId, qint64 userId, QString &error) override {
        deleteCalls.append({mediaId, userId});
        if (!deleteSucceeds) {
            error = QStringLiteral("delete failed");
        }
        return deleteSucceeds;
    }

    bool succeeds = true;
    bool deleteSucceeds = true;
    QList<QPair<int, qint64>> deleteCalls;
    int mediaCalls = 0;
    AniListMediaPendingChanges lastGroup;
};

class AniListPendingChangeProcessorTests : public QObject {
    Q_OBJECT

private slots:
    void compactsLocalWinsLatestByLocalUpdatedAt();
    void groupsChangesByMedia();
    void marksSuccessfulMutationAsSucceeded();
    void preservesFailedMutationForRetry();
    void leavesUnconfirmedDeletionQueued();
    void successfulProcessingClearsPreviousError();
    void sendsPendingDeletionAndSupersedesOtherChangesOfTheMedia();
    void persistsCompactorSupersededRowsAfterDeletion();
    void resendsDeletionLeftInProcessing();
    void failedDeletionIsKeptFailedWithAttemptAndSendsNothingElse();
};

static AniListPendingChange progressChange(AniListPendingChangeStatus status =
                                               AniListPendingChangeStatus::Pending) {
    AniListPendingChange change;
    change.mediaId = 154587;
    change.field = AniListField::Progress;
    change.newValue = 13;
    change.status = status;
    return change;
}

void AniListPendingChangeProcessorTests::compactsLocalWinsLatestByLocalUpdatedAt() {
    auto older = progressChange();
    older.field = AniListField::ListStatus;
    older.id = 1;
    older.localUpdatedAt = QDateTime::fromString(QStringLiteral("2026-09-23T10:00:00"), Qt::ISODate);
    older.newValue = QStringLiteral("WATCHING");

    auto newer = older;
    newer.id = 2;
    newer.localUpdatedAt = QDateTime::fromString(QStringLiteral("2026-09-23T11:00:00"), Qt::ISODate);
    newer.newValue = QStringLiteral("COMPLETED");

    const auto compacted = AniListPendingChangeCompactor::Compact({newer, older});
    QCOMPARE(compacted.size(), 2);
    QCOMPARE(compacted.first().id, qint64(1));
    QCOMPARE(compacted.first().status, AniListPendingChangeStatus::Superseded);
    QCOMPARE(compacted.last().id, qint64(2));
}

void AniListPendingChangeProcessorTests::groupsChangesByMedia() {
    auto firstMediaProgress = progressChange();
    firstMediaProgress.mediaId = 2;
    firstMediaProgress.id = 1;
    firstMediaProgress.localUpdatedAt = QDateTime::fromString(
        QStringLiteral("2026-09-23T10:00:00"), Qt::ISODate);

    auto firstMediaStatus = firstMediaProgress;
    firstMediaStatus.id = 2;
    firstMediaStatus.field = AniListField::ListStatus;
    firstMediaStatus.newValue = QStringLiteral("WATCHING");

    auto secondMediaProgress = firstMediaProgress;
    secondMediaProgress.mediaId = 1;
    secondMediaProgress.id = 3;

    const auto groups = AniListPendingChangeGrouper::Group(
        {firstMediaProgress, firstMediaStatus, secondMediaProgress});
    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups.first().mediaId, 1);
    QCOMPARE(groups.first().changes.size(), 1);
    QCOMPARE(groups.last().mediaId, 2);
    QCOMPARE(groups.last().changes.size(), 2);
}

void AniListPendingChangeProcessorTests::marksSuccessfulMutationAsSucceeded() {
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    QString error;

    QVERIFY(processor.process(154587, error));
    QCOMPARE(client.mediaCalls, 1);
    QCOMPARE(repository.statuses.size(), 2);
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Succeeded);
}

void AniListPendingChangeProcessorTests::preservesFailedMutationForRetry() {
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    client.succeeds = false;
    AniListPendingChangeProcessor processor(repository, client);
    QString error;

    QVERIFY(!processor.process(154587, error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Failed);
}

void AniListPendingChangeProcessorTests::leavesUnconfirmedDeletionQueued() {
    FakePendingRepository repository;
    auto change = progressChange(AniListPendingChangeStatus::RequiresConfirmation);
    change.field = AniListField::Deletion;
    repository.changes.append(change);
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client, 7);
    QString error;

    QVERIFY(processor.process(154587, error));
    QVERIFY(error.isEmpty());
    QCOMPARE(client.mediaCalls, 0);
    QCOMPARE(client.deleteCalls.size(), 0);
    QCOMPARE(repository.statuses.size(), 0);
}

void AniListPendingChangeProcessorTests::successfulProcessingClearsPreviousError() {
    FakePendingRepository repository;
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    QString error = QStringLiteral("stale error");

    QVERIFY(processor.process(154587, error));
    QVERIFY(error.isEmpty());
}

void AniListPendingChangeProcessorTests::sendsPendingDeletionAndSupersedesOtherChangesOfTheMedia() {
    FakePendingRepository repository;
    auto progress = progressChange();
    progress.id = 1;
    auto deletion = progressChange();
    deletion.id = 2;
    deletion.field = AniListField::Deletion;
    repository.changes = {progress, deletion};
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client, 42);
    QString error;

    QVERIFY2(processor.process(154587, error), qPrintable(error));
    QCOMPARE(client.mediaCalls, 0);
    QCOMPARE(client.deleteCalls.size(), 1);
    QCOMPARE(client.deleteCalls.first().first, 154587);
    QCOMPARE(client.deleteCalls.first().second, qint64(42));
    QVERIFY(repository.statuses.contains(AniListPendingChangeStatus::Succeeded));
    QVERIFY(repository.statuses.contains(AniListPendingChangeStatus::Superseded));
    QCOMPARE(repository.statuses.count(AniListPendingChangeStatus::Superseded), 1);
}

void AniListPendingChangeProcessorTests::failedDeletionIsKeptFailedWithAttemptAndSendsNothingElse() {
    FakePendingRepository repository;
    auto progress = progressChange();
    progress.id = 1;
    auto deletion = progressChange(AniListPendingChangeStatus::Failed);
    deletion.id = 2;
    deletion.field = AniListField::Deletion;
    deletion.attempts = 1;
    repository.changes = {progress, deletion};
    FakeUpdateClient client;
    client.deleteSucceeds = false;
    AniListPendingChangeProcessor processor(repository, client, 42);
    QString error;

    QVERIFY(!processor.process(154587, error));
    QCOMPARE(error, QStringLiteral("delete failed"));
    QCOMPARE(client.mediaCalls, 0);
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Failed);
    QCOMPARE(repository.updates.last().attempts, 2);
    QCOMPARE(repository.updates.last().lastError, QStringLiteral("delete failed"));
    QVERIFY(!repository.statuses.contains(AniListPendingChangeStatus::Superseded));
}

void AniListPendingChangeProcessorTests::persistsCompactorSupersededRowsAfterDeletion() {
    FakePendingRepository repository;
    auto first = progressChange();
    first.id = 1;
    first.localUpdatedAt = QDateTime::fromString(QStringLiteral("2026-09-23T10:00:00"), Qt::ISODate);
    auto second = first;
    second.id = 2;
    second.newValue = 14;
    second.localUpdatedAt = QDateTime::fromString(QStringLiteral("2026-09-23T11:00:00"), Qt::ISODate);
    auto deletion = progressChange();
    deletion.id = 3;
    deletion.field = AniListField::Deletion;
    deletion.localUpdatedAt = QDateTime::fromString(QStringLiteral("2026-09-23T12:00:00"), Qt::ISODate);
    repository.changes = {first, second, deletion};
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client, 42);
    QString error;

    QVERIFY2(processor.process(154587, error), qPrintable(error));
    QSet<qint64> supersededIds;
    for (const auto &update : repository.updates) {
        if (update.status == AniListPendingChangeStatus::Superseded) supersededIds.insert(update.id);
    }
    QCOMPARE(supersededIds, QSet<qint64>({1, 2}));
}

void AniListPendingChangeProcessorTests::resendsDeletionLeftInProcessing() {
    FakePendingRepository repository;
    auto deletion = progressChange(AniListPendingChangeStatus::Processing);
    deletion.id = 5;
    deletion.field = AniListField::Deletion;
    repository.changes = {deletion};
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client, 42);
    QString error;

    QVERIFY2(processor.process(154587, error), qPrintable(error));
    QCOMPARE(client.deleteCalls.size(), 1);
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Succeeded);
}

QTEST_MAIN(AniListPendingChangeProcessorTests)
#include "AniListPendingChangeProcessorTests.moc"
