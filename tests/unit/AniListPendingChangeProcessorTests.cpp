#include <QtTest>

#include "../../src/application/anilist/AniListPendingChangeProcessor.h"

class FakePendingRepository final : public IPendingChangeRepository {
public:
    bool Enqueue(const AniListPendingChange &change, QString &error) override {
        Q_UNUSED(error)
        changes.append(change);
        return true;
    }

    bool GetPending(int mediaId, QList<AniListPendingChange> &result, QString &error) override {
        Q_UNUSED(error)
        for (const auto &change : changes) {
            if (change.mediaId == mediaId && change.status != AniListPendingChangeStatus::Succeeded) {
                result.append(change);
            }
        }
        return true;
    }

    bool UpdateStatus(const AniListPendingChange &change, QString &error) override {
        Q_UNUSED(error)
        statuses.append(change.status);
        return true;
    }

    QList<AniListPendingChange> changes;
    QList<AniListPendingChangeStatus> statuses;
};

class FakeUpdateClient final : public IAniListUpdateClient {
public:
    bool UpdateProgress(int, int, QString &error) override {
        progressCalls++;
        if (!succeeds) {
            error = QStringLiteral("update failed");
        }
        return succeeds;
    }

    bool UpdateScore(int, double, QString &error) override {
        Q_UNUSED(error)
        scoreCalls++;
        return succeeds;
    }

    bool UpdateListStatus(int, const QString &, QString &error) override {
        Q_UNUSED(error)
        listStatusCalls++;
        return succeeds;
    }

    bool DeleteListEntry(int, QString &error) override {
        Q_UNUSED(error)
        deleteCalls++;
        return succeeds;
    }

    bool succeeds = true;
    int progressCalls = 0;
    int scoreCalls = 0;
    int listStatusCalls = 0;
    int deleteCalls = 0;
};

class AniListPendingChangeProcessorTests : public QObject {
    Q_OBJECT

private slots:
    void marksSuccessfulMutationAsSucceeded();
    void preservesFailedMutationForRetry();
    void blocksUnconfirmedDeletion();
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

void AniListPendingChangeProcessorTests::marksSuccessfulMutationAsSucceeded() {
    FakePendingRepository repository;
    repository.changes.append(progressChange());
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    QString error;

    QVERIFY(processor.Process(154587, error));
    QCOMPARE(client.progressCalls, 1);
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

    QVERIFY(!processor.Process(154587, error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Failed);
}

void AniListPendingChangeProcessorTests::blocksUnconfirmedDeletion() {
    FakePendingRepository repository;
    auto change = progressChange();
    change.field = AniListField::Deletion;
    repository.changes.append(change);
    FakeUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    QString error;

    QVERIFY(!processor.Process(154587, error));
    QCOMPARE(client.deleteCalls, 0);
    QCOMPARE(repository.statuses.last(), AniListPendingChangeStatus::Failed);
}

QTEST_MAIN(AniListPendingChangeProcessorTests)
#include "AniListPendingChangeProcessorTests.moc"
