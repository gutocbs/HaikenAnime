#include <QtTest>

#include "../../src/application/anilist/AniListPendingChangeReconciler.h"

namespace {
class PendingRepository final : public IPendingChangeRepository {
public:
    bool enqueue(const AniListPendingChange &change, QString &error) override {
        changes.append(change);
        error.clear();
        return true;
    }

    bool getPending(const int mediaId, QList<AniListPendingChange> &result,
                    QString &error) override {
        result.clear();
        for (const auto &change : changes) {
            if (change.mediaId == mediaId
                && change.status != AniListPendingChangeStatus::Succeeded
                && change.status != AniListPendingChangeStatus::Superseded) {
                result.append(change);
            }
        }
        error.clear();
        return true;
    }

    bool getPendingMediaIds(QList<int> &result, QString &error) override {
        result.clear();
        error.clear();
        return true;
    }

    bool updateStatus(const AniListPendingChange &updated, QString &error) override {
        for (auto &change : changes) {
            if (change.id == updated.id) {
                change = updated;
                error.clear();
                return true;
            }
        }
        error = QStringLiteral("Pending change not found.");
        return false;
    }

    QList<AniListPendingChange> changes;
};

AniListPendingChange progressChange(const int value) {
    AniListPendingChange change;
    change.id = 1;
    change.mediaId = 42;
    change.field = AniListField::Progress;
    change.previousValue = 9;
    change.newValue = value;
    change.createdAt = QDateTime::fromString(QStringLiteral("2026-10-10T10:00:00Z"), Qt::ISODate);
    change.localUpdatedAt = change.createdAt;
    return change;
}

Media remoteMedia(const int progress) {
    Media media;
    media.Id = 42;
    media.ConsumedChapters = progress;
    media.ListStatus = UserListStatus::Completed;
    return media;
}
}

class AniListPendingChangeReconcilerTests final : public QObject {
    Q_OBJECT

private slots:
    void supersedesLowerLocalProgressWhenRemoteIsAhead();
    void completesPendingProgressAlreadyConfirmedRemotely();
    void preservesHigherLocalProgressForMutationAndPersistence();
    void compactsSuccessiveScoreChangesBeforeReconciliation();
    void logsReconciliationOutcomeWithoutPendingValues();
    void preservesLocalScoreAndStatusForMutation();
};

void AniListPendingChangeReconcilerTests::supersedesLowerLocalProgressWhenRemoteIsAhead() {
    PendingRepository repository;
    repository.changes.append(progressChange(10));
    AniListPendingChangeReconciler reconciler(repository);
    QList<Media> media{remoteMedia(12)};
    QString error;

    QVERIFY2(reconciler.reconcile(media, error), qPrintable(error));
    QCOMPARE(media.first().ConsumedChapters, 12);
    QCOMPARE(repository.changes.first().status, AniListPendingChangeStatus::Superseded);
}

void AniListPendingChangeReconcilerTests::completesPendingProgressAlreadyConfirmedRemotely() {
    PendingRepository repository;
    repository.changes.append(progressChange(10));
    AniListPendingChangeReconciler reconciler(repository);
    QList<Media> media{remoteMedia(10)};
    QString error;

    QVERIFY2(reconciler.reconcile(media, error), qPrintable(error));
    QCOMPARE(media.first().ConsumedChapters, 10);
    QCOMPARE(repository.changes.first().status, AniListPendingChangeStatus::Succeeded);
}

void AniListPendingChangeReconcilerTests::preservesHigherLocalProgressForMutationAndPersistence() {
    PendingRepository repository;
    repository.changes.append(progressChange(10));
    AniListPendingChangeReconciler reconciler(repository);
    QList<Media> media{remoteMedia(8)};
    QString error;

    QVERIFY2(reconciler.reconcile(media, error), qPrintable(error));
    QCOMPARE(media.first().ConsumedChapters, 10);
    QCOMPARE(repository.changes.first().status, AniListPendingChangeStatus::Pending);
}

void AniListPendingChangeReconcilerTests::compactsSuccessiveScoreChangesBeforeReconciliation() {
    PendingRepository repository;
    auto first = progressChange(7);
    first.field = AniListField::PersonalScore;
    first.newValue = 7;
    auto latest = first;
    latest.id = 2;
    latest.newValue = 9;
    latest.createdAt = first.createdAt.addSecs(1);
    latest.localUpdatedAt = latest.createdAt;
    repository.changes = {first, latest};

    auto mediaItem = remoteMedia(12);
    mediaItem.PersonalScore = 9;
    QList<Media> media{mediaItem};
    AniListPendingChangeReconciler reconciler(repository);
    QString error;

    QVERIFY2(reconciler.reconcile(media, error), qPrintable(error));
    QCOMPARE(media.first().PersonalScore, 9);
    QCOMPARE(repository.changes.at(0).status, AniListPendingChangeStatus::Superseded);
    QCOMPARE(repository.changes.at(1).status, AniListPendingChangeStatus::Succeeded);
}

void AniListPendingChangeReconcilerTests::logsReconciliationOutcomeWithoutPendingValues() {
    PendingRepository repository;
    repository.changes.append(progressChange(10));
    QStringList events;
    AniListPendingChangeReconciler reconciler(
        repository, [&events](const QString &event) { events.append(event); });
    QList<Media> media{remoteMedia(12)};
    QString error;

    QVERIFY2(reconciler.reconcile(media, error), qPrintable(error));
    QCOMPARE(events.size(), 2);
    QCOMPARE(events.first(),
             QStringLiteral("AniList pending-change reconciliation started: media=1."));
    QCOMPARE(events.last(),
             QStringLiteral("AniList pending-change reconciliation completed: examined=1 retired=1 preserved=0."));
    QVERIFY(!events.join(' ').contains(QStringLiteral("value"), Qt::CaseInsensitive));
}

void AniListPendingChangeReconcilerTests::preservesLocalScoreAndStatusForMutation() {
    PendingRepository repository;
    auto score = progressChange(9);
    score.field = AniListField::PersonalScore;
    score.newValue = 9;
    auto status = score;
    status.id = 2;
    status.field = AniListField::ListStatus;
    status.newValue = QStringLiteral("COMPLETED");
    status.createdAt = score.createdAt.addSecs(1);
    status.localUpdatedAt = status.createdAt;
    repository.changes = {score, status};

    auto mediaItem = remoteMedia(12);
    mediaItem.PersonalScore = 7;
    mediaItem.ListStatus = UserListStatus::Current;
    QList<Media> media{mediaItem};
    AniListPendingChangeReconciler reconciler(repository);
    QString error;

    QVERIFY2(reconciler.reconcile(media, error), qPrintable(error));
    QCOMPARE(media.first().PersonalScore, 9);
    QCOMPARE(media.first().ListStatus, UserListStatus::Completed);
    QCOMPARE(repository.changes.at(0).status, AniListPendingChangeStatus::Pending);
    QCOMPARE(repository.changes.at(1).status, AniListPendingChangeStatus::Pending);
}

QTEST_MAIN(AniListPendingChangeReconcilerTests)
#include "AniListPendingChangeReconcilerTests.moc"
