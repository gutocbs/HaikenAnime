#include <QtTest>

#include <optional>

#include "../../src/application/anilist/AniListPendingChangeProcessor.h"
#include "../../src/application/anilist/AniListPendingChangeCompactor.h"
#include "../../src/application/anilist/AniListPendingChangeGrouper.h"
#include "../../src/application/anilist/IAniListPendingChangeObserver.h"

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
        if (failOnStatus.has_value() && change.status == *failOnStatus) {
            error = QStringLiteral("status write failed");
            return false;
        }
        statuses.append(change.status);
        updates.append(change);
        return true;
    }

    QList<AniListPendingChange> changes;
    std::optional<AniListPendingChangeStatus> failOnStatus;
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
            error = failureMessage;
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
    QString failureMessage = QStringLiteral("update failed");
    bool deleteSucceeds = true;
    QList<QPair<int, qint64>> deleteCalls;
    int mediaCalls = 0;
    AniListMediaPendingChanges lastGroup;
};

class RecordingObserver final : public IAniListPendingChangeObserver {
public:
    void changesSent(const QList<AniListPendingChange> &changes) override { sent.append(changes); }
    void changesFailed(const QList<AniListPendingChange> &changes,
                       const AniListSyncErrorCategory category) override {
        failed.append(changes);
        categories.append(category);
    }

    QList<QList<AniListPendingChange>> sent;
    QList<QList<AniListPendingChange>> failed;
    QList<AniListSyncErrorCategory> categories;
};

/** Records which outbox status was already persisted when the observer was notified. */
class OrderingObserver final : public IAniListPendingChangeObserver {
public:
    explicit OrderingObserver(const FakePendingRepository &repository) : repository_(repository) {}
    void changesSent(const QList<AniListPendingChange> &) override { lastStatus = repository_.statuses.last(); }
    void changesFailed(const QList<AniListPendingChange> &, AniListSyncErrorCategory) override {
        lastStatus = repository_.statuses.last();
    }

    std::optional<AniListPendingChangeStatus> lastStatus;

private:
    const FakePendingRepository &repository_;
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
    void deletionStaysResendableWhenSupersedingSiblingsFails();
    void logsDeletionSentAndSupersededCountWithoutPayloads();
    void logsDeletionFailureWithSafeError();
    void cancelledUpdateLeavesChangeAsItWasWithoutCountingAnAttempt();
    void cancelledDeletionLeavesChangeAsItWasWithoutCountingAnAttempt();
    void notifiesObserverAfterSentChangesArePersisted();
    void notifiesObserverOfRetryableFailureAfterFailedStatusIsPersisted();
    void notifiesObserverOfDefinitiveFailureWithCategoryOnly();
    void doesNotNotifyObserverWhenUpdateIsCancelled();
    void doesNotNotifyObserverWhenSucceededStatusCannotBePersisted();
    void observerDoesNotChangeOutboxStatuses();
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

void AniListPendingChangeProcessorTests::deletionStaysResendableWhenSupersedingSiblingsFails() {
    FakePendingRepository repository;
    auto progress = progressChange();
    progress.id = 1;
    auto deletion = progressChange();
    deletion.id = 2;
    deletion.field = AniListField::Deletion;
    repository.changes = {progress, deletion};
    repository.failOnStatus = AniListPendingChangeStatus::Superseded;
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client, 42);
    QString error;

    QVERIFY(!processor.process(154587, error));
    QCOMPARE(error, QStringLiteral("status write failed"));
    // The deletion was never marked Succeeded, so the next drain resends it (idempotent).
    QVERIFY(!repository.statuses.contains(AniListPendingChangeStatus::Succeeded));
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Processing);
}

void AniListPendingChangeProcessorTests::logsDeletionSentAndSupersededCountWithoutPayloads() {
    FakePendingRepository repository;
    auto progress = progressChange();
    progress.id = 1;
    progress.newValue = 987654;
    auto deletion = progressChange();
    deletion.id = 2;
    deletion.field = AniListField::Deletion;
    repository.changes = {progress, deletion};
    FakeUpdateClient client;
    QStringList logs;
    AniListPendingChangeProcessor processor(repository, client, 42,
                                            [&logs](const QString &line) { logs.append(line); });
    QString error;

    QVERIFY2(processor.process(154587, error), qPrintable(error));
    QCOMPARE(logs, QStringList({
        QStringLiteral("AniList list deletion sent for media 154587."),
        QStringLiteral("Superseded 1 pending AniList changes for media 154587 after list deletion.")}));
}

void AniListPendingChangeProcessorTests::logsDeletionFailureWithSafeError() {
    FakePendingRepository repository;
    auto deletion = progressChange();
    deletion.id = 2;
    deletion.field = AniListField::Deletion;
    repository.changes = {deletion};
    FakeUpdateClient client;
    client.deleteSucceeds = false;
    QStringList logs;
    AniListPendingChangeProcessor processor(repository, client, 42,
                                            [&logs](const QString &line) { logs.append(line); });
    QString error;

    QVERIFY(!processor.process(154587, error));
    QCOMPARE(logs, QStringList({
        QStringLiteral("AniList list deletion failed for media 154587: delete failed.")}));
}

void AniListPendingChangeProcessorTests::cancelledUpdateLeavesChangeAsItWasWithoutCountingAnAttempt() {
    FakePendingRepository repository;
    auto change = progressChange(AniListPendingChangeStatus::Failed);
    change.attempts = 2;
    change.lastError = QStringLiteral("earlier failure");
    repository.changes.append(change);
    FakeUpdateClient client;
    client.succeeds = false;
    AniListPendingChangeProcessor processor(repository, client, 0, {}, [] { return true; });
    QString error;

    QVERIFY(!processor.process(154587, error));

    QVERIFY(!repository.updates.isEmpty());
    const auto &last = repository.updates.last();
    QCOMPARE(last.status, AniListPendingChangeStatus::Failed);
    QCOMPARE(last.attempts, 2);
    QCOMPARE(last.lastError, QStringLiteral("earlier failure"));
}

void AniListPendingChangeProcessorTests::cancelledDeletionLeavesChangeAsItWasWithoutCountingAnAttempt() {
    FakePendingRepository repository;
    auto change = progressChange(AniListPendingChangeStatus::Pending);
    change.field = AniListField::Deletion;
    repository.changes.append(change);
    FakeUpdateClient client;
    client.deleteSucceeds = false;
    AniListPendingChangeProcessor processor(repository, client, 7, {}, [] { return true; });
    QString error;

    QVERIFY(!processor.process(154587, error));

    QVERIFY(!repository.updates.isEmpty());
    const auto &last = repository.updates.last();
    QCOMPARE(last.status, AniListPendingChangeStatus::Pending);
    QCOMPARE(last.attempts, 0);
    QVERIFY(last.lastError.isEmpty());
}

void AniListPendingChangeProcessorTests::notifiesObserverAfterSentChangesArePersisted() {
    // Arrange
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    RecordingObserver observer;
    OrderingObserver ordering(repository);
    processor.setObserver(&observer);
    QString error;

    // Act
    QVERIFY(processor.process(154587, error));
    processor.setObserver(&ordering);
    QVERIFY(processor.process(154587, error));

    // Assert
    QCOMPARE(observer.sent.size(), 1);
    QCOMPARE(observer.sent.first().size(), 1);
    QCOMPARE(observer.sent.first().first().field, AniListField::Progress);
    QCOMPARE(std::get<int>(observer.sent.first().first().newValue), 13);
    QVERIFY(observer.failed.isEmpty());
    QCOMPARE(ordering.lastStatus, std::optional(AniListPendingChangeStatus::Succeeded));
}

void AniListPendingChangeProcessorTests::notifiesObserverOfRetryableFailureAfterFailedStatusIsPersisted() {
    // Arrange
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    client.succeeds = false;
    client.failureMessage = QStringLiteral("AniList network connection refused");
    AniListPendingChangeProcessor processor(repository, client);
    RecordingObserver observer;
    OrderingObserver ordering(repository);
    processor.setObserver(&observer);
    QString error;

    // Act
    QVERIFY(!processor.process(154587, error));
    processor.setObserver(&ordering);
    QVERIFY(!processor.process(154587, error));

    // Assert
    QCOMPARE(observer.failed.size(), 1);
    QCOMPARE(observer.failed.first().first().mediaId, 154587);
    QCOMPARE(observer.categories, QList{AniListSyncErrorCategory::Network});
    QVERIFY(IsRetryable(observer.categories.first()));
    QVERIFY(observer.sent.isEmpty());
    QCOMPARE(ordering.lastStatus, std::optional(AniListPendingChangeStatus::Failed));
}

void AniListPendingChangeProcessorTests::notifiesObserverOfDefinitiveFailureWithCategoryOnly() {
    // Arrange
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    client.succeeds = false;
    client.failureMessage = QStringLiteral("Unauthorized: Bearer secret-token-123 was rejected");
    AniListPendingChangeProcessor processor(repository, client);
    RecordingObserver observer;
    processor.setObserver(&observer);
    QString error;

    // Act
    QVERIFY(!processor.process(154587, error));

    // Assert: only the category crosses the boundary; the raw (token-bearing) message stays in the outbox.
    QCOMPARE(observer.categories, QList{AniListSyncErrorCategory::Authentication});
    QVERIFY(!IsRetryable(observer.categories.first()));
    QVERIFY(observer.failed.first().first().lastError.isEmpty());
}

void AniListPendingChangeProcessorTests::doesNotNotifyObserverWhenUpdateIsCancelled() {
    // Arrange
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    client.succeeds = false;
    AniListPendingChangeProcessor processor(repository, client, 0, {}, [] { return true; });
    RecordingObserver observer;
    processor.setObserver(&observer);
    QString error;

    // Act
    QVERIFY(!processor.process(154587, error));

    // Assert
    QVERIFY(observer.sent.isEmpty());
    QVERIFY(observer.failed.isEmpty());
}

void AniListPendingChangeProcessorTests::doesNotNotifyObserverWhenSucceededStatusCannotBePersisted() {
    // Arrange
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    repository.failOnStatus = AniListPendingChangeStatus::Succeeded;
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    RecordingObserver observer;
    processor.setObserver(&observer);
    QString error;

    // Act
    QVERIFY(!processor.process(154587, error));

    // Assert: the change is resent later, so nothing is reported as synchronized yet.
    QVERIFY(observer.sent.isEmpty());
    QVERIFY(observer.failed.isEmpty());
}

void AniListPendingChangeProcessorTests::observerDoesNotChangeOutboxStatuses() {
    // Arrange
    FakePendingRepository withoutObserver;
    withoutObserver.changes.append(progressChange());
    FakePendingRepository withObserver;
    withObserver.changes.append(progressChange());
    FakeUpdateClient client;
    client.succeeds = false;
    AniListPendingChangeProcessor plain(withoutObserver, client);
    AniListPendingChangeProcessor observed(withObserver, client);
    RecordingObserver observer;
    observed.setObserver(&observer);
    QString plainError;
    QString observedError;

    // Act
    const bool plainResult = plain.process(154587, plainError);
    const bool observedResult = observed.process(154587, observedError);

    // Assert
    QCOMPARE(observedResult, plainResult);
    QCOMPARE(observedError, plainError);
    QCOMPARE(withObserver.statuses, withoutObserver.statuses);
    QCOMPARE(withObserver.updates.last().attempts, withoutObserver.updates.last().attempts);
    QCOMPARE(withObserver.updates.last().lastError, withoutObserver.updates.last().lastError);
}

QTEST_MAIN(AniListPendingChangeProcessorTests)
#include "AniListPendingChangeProcessorTests.moc"
