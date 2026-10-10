#include <QtTest>

#include "../../src/application/anilist/AniListPendingChangeDrain.h"

namespace {
AniListPendingChange pendingChange(const qint64 id, const int mediaId) {
    AniListPendingChange change;
    change.id = id;
    change.mediaId = mediaId;
    change.field = AniListField::Progress;
    change.newValue = mediaId;
    change.createdAt = QDateTime::fromMSecsSinceEpoch(id, QTimeZone::UTC);
    change.localUpdatedAt = change.createdAt;
    return change;
}

class GrowingPendingRepository final : public IPendingChangeRepository {
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
                && (change.status == AniListPendingChangeStatus::Pending
                    || change.status == AniListPendingChangeStatus::Processing
                    || change.status == AniListPendingChangeStatus::Failed
                    || change.status == AniListPendingChangeStatus::RequiresConfirmation)) {
                result.append(change);
            }
        }
        error.clear();
        return true;
    }

    bool getPendingMediaIds(QList<int> &result, QString &error) override {
        result.clear();
        for (const auto &change : changes) {
            if ((change.status == AniListPendingChangeStatus::Pending
                 || change.status == AniListPendingChangeStatus::Processing
                 || change.status == AniListPendingChangeStatus::Failed)
                && !result.contains(change.mediaId)) {
                result.append(change.mediaId);
            }
        }
        error.clear();
        return true;
    }

    bool updateStatus(const AniListPendingChange &updated, QString &error) override {
        for (auto &change : changes) {
            if (change.id == updated.id) {
                change = updated;
                if (updated.id == 1 && updated.status == AniListPendingChangeStatus::Succeeded
                    && !secondMediaInserted) {
                    changes.append(pendingChange(2, 22));
                    secondMediaInserted = true;
                }
                error.clear();
                return true;
            }
        }
        error = QStringLiteral("missing change");
        return false;
    }

    QList<AniListPendingChange> changes{pendingChange(1, 11)};
    bool secondMediaInserted = false;
};

class RecordingUpdateClient final : public IAniListUpdateClient {
public:
    bool updateMedia(const AniListMediaPendingChanges &changes, QString &error) override {
        mediaIds.append(changes.mediaId);
        error.clear();
        return true;
    }

    QList<int> mediaIds;
};
}

class AniListPendingChangeDrainTests final : public QObject {
    Q_OBJECT

private slots:
    void includesChangesInsertedWhileTheCurrentDrainIsRunning();
};

void AniListPendingChangeDrainTests::includesChangesInsertedWhileTheCurrentDrainIsRunning() {
    GrowingPendingRepository repository;
    RecordingUpdateClient client;
    AniListPendingChangeProcessor processor(repository, client);
    AniListPendingChangeDrain drain(repository, processor);
    QString error;

    QVERIFY2(drain.processAll(error), qPrintable(error));
    QCOMPARE(client.mediaIds, QList<int>({11, 22}));
}

QTEST_MAIN(AniListPendingChangeDrainTests)
#include "AniListPendingChangeDrainTests.moc"
