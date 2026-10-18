#include <QtTest>

#include "../../src/application/media/IPersonalListChangeWriter.h"
#include "../../src/application/recognition/PersonalListProgressWriter.h"
#include "../../src/application/recognition/RecognitionHistoryService.h"

namespace {
const QDateTime Now(QDate(2026, 10, 17), QTime(21, 0), QTimeZone::UTC);

class FakeHistoryRepository final : public IRecognitionHistoryRepository {
public:
    bool insert(RecognitionRecord &record, QString &error) override {
        error.clear();
        for (const auto &stored : records) {
            if (stored.deduplicationKey() == record.deduplicationKey()) {
                error = QStringLiteral("duplicate");
                return false;
            }
        }
        record.assignId(++lastId);
        records.append(record);
        return true;
    }

    bool update(const RecognitionRecord &record, QString &error) override {
        error.clear();
        ++updates;
        for (auto &stored : records) {
            if (stored.id() == record.id()) {
                stored = record;
                return true;
            }
        }
        error = QStringLiteral("missing");
        return false;
    }

    bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &found, QString &error) override {
        error.clear();
        lastQuery = query;
        found.clear();
        for (auto record = records.crbegin(); record != records.crend(); ++record) {
            if (query.recordId && record->id() != *query.recordId) continue;
            if (!query.deduplicationKey.isEmpty() && record->deduplicationKey() != query.deduplicationKey) continue;
            if (query.status && record->status() != *query.status) continue;
            if (query.beforeId && record->id() >= *query.beforeId) continue;
            if (found.size() >= query.limit) break;
            found.append(*record);
        }
        return true;
    }

    bool remove(const qint64 recordId, const std::optional<QueuedProgress> &queuedProgress, QString &error) override {
        if (failRemove) {
            error = QStringLiteral("database is locked");
            return false;
        }
        error.clear();
        removedQueuedProgress = queuedProgress;
        return records.removeIf([recordId](const RecognitionRecord &record) { return record.id() == recordId; }) > 0;
    }

    bool hasQueuedProgress(const QueuedProgress &progress, bool &found, QString &error) override {
        error.clear();
        found = queuedMediaIds.contains(progress.mediaId);
        return true;
    }

    bool failRemove = false;
    std::optional<QueuedProgress> removedQueuedProgress;
    QSet<int> queuedMediaIds;
    QList<RecognitionRecord> records;
    qint64 lastId = 0;
    int updates = 0;
    RecognitionHistoryQuery lastQuery;
};

class FakeProgressWriter final : public IRecognitionProgressWriter {
public:
    bool apply(const Media &previous, const Media &updated, QString &error) override {
        if (onApply) onApply();
        if (fail) {
            error = QStringLiteral("database is locked");
            return false;
        }
        error.clear();
        applied.append({previous.ConsumedChapters, updated.ConsumedChapters});
        return true;
    }

    QList<QPair<int, int>> applied;
    bool fail = false;
    std::function<void()> onApply;
};

class RecordingChangeWriter final : public IPersonalListChangeWriter {
public:
    bool remove(const Media &, const AniListPendingChange &, QString &) override { return true; }
    bool save(const Media &media, const QList<AniListPendingChange> &changes, QString &error) override {
        error.clear();
        savedMedia = media;
        savedChanges = changes;
        return true;
    }

    Media savedMedia;
    QList<AniListPendingChange> savedChanges;
};

PlaybackSession session(const QString &key = QStringLiteral("abc")) {
    return {.id = PlaybackSessionId::create(QStringLiteral("plex"), key).value(), .userId = QStringLiteral("7"),
            .deviceId = QStringLiteral("tv"), .title = QStringLiteral("One Piece"), .startedAt = Now.addSecs(-300)};
}

EpisodeCandidate candidate(const int episode, const double confidence = 1.0) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, episode, confidence).value();
}

Media media(const int consumed) {
    Media value;
    value.Id = 21;
    value.ConsumedChapters = consumed;
    value.TotalChapters = 12;
    value.ListStatus = UserListStatus::Current;
    return value;
}

ProgressDecision markWatched(const int from, const int to) {
    auto updated = media(to);
    return {.kind = ProgressDecisionKind::MarkWatched, .previous = media(from), .updated = updated};
}
}

class RecognitionHistoryServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void markWatchedAppliesProgressAndQueuesSync();
    void alreadyWatchedIsRecordedWithoutProgress();
    void uncertainDecisionIsRecordedWithDiagnostic();
    void ignoredDecisionStaysCorrectableAsUncertain();
    void duplicateSessionEpisodeNeverAppliesProgressTwice();
    void incompleteObservationIsRecordedWithoutProgress();
    void completionAfterIncompleteReusesTheRecord();
    void progressWriteFailureKeepsRecordCorrectableWithoutSync();
    void syncFailureIsDurableAndKeepsLocalProgress();
    void correctionAppliesCorrectedProgress();
    void ignoreMarksRecordIgnored();
    void correctingUnknownRecordFails();
    void listForwardsFilterAndCursor();
    void personalListWriterQueuesAniListProgressChange();
    void personalListWriterNotifiesOnlyAfterSuccessfulApply();
    void removeDiscardsQueuedProgressAndKeepsLocalProgress();
    void removeWithoutQueuedSyncDoesNotTouchTheQueue();
    void removeSyncedRecordDoesNotTouchTheQueue();
    void removeFailureKeepsTheRecord();
    void removeUnknownRecordFails();
    void recordIsAwaitingSyncBeforeProgressIsQueued();
    void failedQueueingRevertsRecordToUncertain();
    void recoveryTurnsInterruptedPendingRecordsUncertain();
    void recoveryTurnsUnqueuedSyncPendingRecordsUncertain();
    void recoveryKeepsQueuedSyncPendingRecords();

private:
    FakeHistoryRepository repository;
    FakeProgressWriter progress;
    std::unique_ptr<RecognitionHistoryService> service;
};

void RecognitionHistoryServiceTests::init() {
    repository = FakeHistoryRepository();
    progress = FakeProgressWriter();
    service = std::make_unique<RecognitionHistoryService>(repository, progress, [] { return Now; });
}

void RecognitionHistoryServiceTests::markWatchedAppliesProgressAndQueuesSync() {
    QString error;

    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);

    QVERIFY2(record.has_value(), qPrintable(error));
    QCOMPARE(record->status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(record->syncState(), RecognitionSyncState::Pending);
    QVERIFY(record->id() > 0);
    QCOMPARE(progress.applied, (QList<QPair<int, int>>{{5, 6}}));
    QCOMPARE(repository.records.size(), 1);
}

void RecognitionHistoryServiceTests::alreadyWatchedIsRecordedWithoutProgress() {
    QString error;

    const auto record = service->record(session(), candidate(3),
                                        {.kind = ProgressDecisionKind::AlreadyWatched, .previous = media(5)}, error);

    QVERIFY(record.has_value());
    QCOMPARE(record->status(), RecognitionStatus::AlreadyWatched);
    QCOMPARE(record->syncState(), RecognitionSyncState::NotRequired);
    QVERIFY(progress.applied.isEmpty());
}

void RecognitionHistoryServiceTests::uncertainDecisionIsRecordedWithDiagnostic() {
    QString error;

    const auto record = service->record(session(), candidate(3, 0.4),
                                        {.kind = ProgressDecisionKind::Uncertain,
                                         .reason = QStringLiteral("low-confidence")}, error);

    QVERIFY(record.has_value());
    QCOMPARE(record->status(), RecognitionStatus::Uncertain);
    QCOMPARE(record->diagnostic(), QStringLiteral("low-confidence"));
    QVERIFY(progress.applied.isEmpty());
}

void RecognitionHistoryServiceTests::ignoredDecisionStaysCorrectableAsUncertain() {
    QString error;

    const auto record = service->record(session(), candidate(3),
                                        {.kind = ProgressDecisionKind::Ignored,
                                         .reason = QStringLiteral("media-not-in-library")}, error);

    QVERIFY(record.has_value());
    QCOMPARE(record->status(), RecognitionStatus::Uncertain);
    QCOMPARE(record->diagnostic(), QStringLiteral("media-not-in-library"));
}

void RecognitionHistoryServiceTests::duplicateSessionEpisodeNeverAppliesProgressTwice() {
    QString error;
    const auto first = service->record(session(), candidate(6), markWatched(5, 6), error);
    QVERIFY2(first.has_value(), qPrintable(error));

    const auto repeated = service->record(session(), candidate(6, 0.9), markWatched(5, 6), error);

    QVERIFY(repeated.has_value());
    QCOMPARE(repeated->id(), first->id());
    QCOMPARE(progress.applied.size(), 1);
    QCOMPARE(repository.records.size(), 1);
}

void RecognitionHistoryServiceTests::incompleteObservationIsRecordedWithoutProgress() {
    QString error;

    const auto record = service->recordIncomplete(session(), candidate(6), error);

    QVERIFY(record.has_value());
    QCOMPARE(record->status(), RecognitionStatus::Incomplete);
    QVERIFY(progress.applied.isEmpty());
}

void RecognitionHistoryServiceTests::completionAfterIncompleteReusesTheRecord() {
    QString error;
    const auto incomplete = service->recordIncomplete(session(), candidate(6), error);
    QVERIFY2(incomplete.has_value(), qPrintable(error));

    const auto completed = service->record(session(), candidate(6), markWatched(5, 6), error);

    QVERIFY2(completed.has_value(), qPrintable(error));
    QCOMPARE(completed->id(), incomplete->id());
    QCOMPARE(completed->status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(repository.records.size(), 1);
    QCOMPARE(repository.records.first().status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(progress.applied.size(), 1);
}

void RecognitionHistoryServiceTests::progressWriteFailureKeepsRecordCorrectableWithoutSync() {
    progress.fail = true;
    QString error;

    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);

    QVERIFY(record.has_value());
    QCOMPARE(record->status(), RecognitionStatus::Uncertain);
    QCOMPARE(record->diagnostic(), QStringLiteral("progress-write-failed"));
    QCOMPARE(record->syncState(), RecognitionSyncState::NotRequired);
}

void RecognitionHistoryServiceTests::syncFailureIsDurableAndKeepsLocalProgress() {
    QString error;
    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);
    QVERIFY2(record.has_value(), qPrintable(error));

    QVERIFY2(service->markSyncFailure(record->id(), QStringLiteral("network"), error), qPrintable(error));

    QCOMPARE(repository.records.first().syncState(), RecognitionSyncState::Failed);
    QCOMPARE(repository.records.first().syncError(), QStringLiteral("network"));
    QCOMPARE(repository.records.first().status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(progress.applied.size(), 1);
    QVERIFY(service->markSynced(record->id(), error));
    QCOMPARE(repository.records.first().syncState(), RecognitionSyncState::Synced);
}

void RecognitionHistoryServiceTests::correctionAppliesCorrectedProgress() {
    QString error;
    const auto record = service->record(session(), candidate(3, 0.4),
                                        {.kind = ProgressDecisionKind::Uncertain}, error);
    QVERIFY2(record.has_value(), qPrintable(error));

    QVERIFY2(service->correct(record->id(), candidate(6), markWatched(5, 6), error), qPrintable(error));

    const auto stored = repository.records.first();
    QCOMPARE(stored.status(), RecognitionStatus::Corrected);
    QCOMPARE(stored.effectiveCandidate().episode(), 6);
    QCOMPARE(stored.syncState(), RecognitionSyncState::Pending);
    QCOMPARE(progress.applied, (QList<QPair<int, int>>{{5, 6}}));
}

void RecognitionHistoryServiceTests::ignoreMarksRecordIgnored() {
    QString error;
    const auto record = service->record(session(), candidate(3, 0.4), {.kind = ProgressDecisionKind::Uncertain}, error);
    QVERIFY2(record.has_value(), qPrintable(error));

    QVERIFY2(service->ignore(record->id(), error), qPrintable(error));

    QCOMPARE(repository.records.first().status(), RecognitionStatus::Ignored);
}

void RecognitionHistoryServiceTests::correctingUnknownRecordFails() {
    QString error;

    QVERIFY(!service->correct(99, candidate(6), markWatched(5, 6), error));
    QVERIFY(!error.isEmpty());
    QVERIFY(progress.applied.isEmpty());
}

void RecognitionHistoryServiceTests::listForwardsFilterAndCursor() {
    QString error;
    for (int episode = 1; episode <= 3; ++episode) {
        QVERIFY(service->record(session(), candidate(episode), {.kind = ProgressDecisionKind::AlreadyWatched}, error));
    }
    QList<RecognitionRecord> records;

    QVERIFY2(service->list({.status = RecognitionStatus::AlreadyWatched, .pluginId = QStringLiteral("plex")},
                           {.limit = 2, .beforeId = 3}, records, error),
             qPrintable(error));

    QCOMPARE(records.size(), 2);
    QCOMPARE(records.at(0).id(), 2);
    QCOMPARE(records.at(1).id(), 1);
    QCOMPARE(repository.lastQuery.pluginId, QStringLiteral("plex"));
    QCOMPARE(repository.lastQuery.limit, 2);
}

void RecognitionHistoryServiceTests::personalListWriterQueuesAniListProgressChange() {
    RecordingChangeWriter changes;
    const PersonalListChangeService changeService(changes);
    PersonalListProgressWriter writer(changeService);
    QString error;

    QVERIFY2(writer.apply(media(5), media(6), error), qPrintable(error));

    QCOMPARE(changes.savedMedia.ConsumedChapters, 6);
    QCOMPARE(changes.savedChanges.size(), 1);
    QCOMPARE(changes.savedChanges.first().field, AniListField::Progress);
    QCOMPARE(changes.savedChanges.first().newValue, AniListFieldValue(6));
}

void RecognitionHistoryServiceTests::personalListWriterNotifiesOnlyAfterSuccessfulApply() {
    RecordingChangeWriter changes;
    const PersonalListChangeService changeService(changes);
    QList<int> notified;
    PersonalListProgressWriter writer(changeService, [&notified](const Media &media) { notified.append(media.Id); });
    QString error;
    Media invalid = media(6);
    invalid.Id = 0;

    QVERIFY(writer.apply(media(5), media(6), error));
    QVERIFY(!writer.apply(media(5), invalid, error));

    QCOMPARE(notified, QList<int>({21}));
}

void RecognitionHistoryServiceTests::removeDiscardsQueuedProgressAndKeepsLocalProgress() {
    QString error;
    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);
    QVERIFY2(record.has_value(), qPrintable(error));

    QVERIFY2(service->remove(record->id(), error), qPrintable(error));

    QVERIFY(repository.records.isEmpty());
    QVERIFY(repository.removedQueuedProgress.has_value());
    QCOMPARE(repository.removedQueuedProgress->mediaId, 21);
    QCOMPARE(repository.removedQueuedProgress->episode, 6);
    QCOMPARE(progress.applied.size(), 1);
}

void RecognitionHistoryServiceTests::removeWithoutQueuedSyncDoesNotTouchTheQueue() {
    QString error;
    const auto record = service->record(session(), candidate(3), {.kind = ProgressDecisionKind::AlreadyWatched}, error);
    QVERIFY2(record.has_value(), qPrintable(error));

    QVERIFY(service->remove(record->id(), error));

    QVERIFY(!repository.removedQueuedProgress.has_value());
    QVERIFY(repository.records.isEmpty());
}

void RecognitionHistoryServiceTests::removeSyncedRecordDoesNotTouchTheQueue() {
    QString error;
    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);
    QVERIFY2(record.has_value(), qPrintable(error));
    QVERIFY(service->markSynced(record->id(), error));

    QVERIFY(service->remove(record->id(), error));

    QVERIFY(!repository.removedQueuedProgress.has_value());
}

void RecognitionHistoryServiceTests::removeFailureKeepsTheRecord() {
    QString error;
    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);
    QVERIFY2(record.has_value(), qPrintable(error));
    repository.failRemove = true;

    QVERIFY(!service->remove(record->id(), error));

    QVERIFY(!error.isEmpty());
    QCOMPARE(repository.records.size(), 1);
}

void RecognitionHistoryServiceTests::removeUnknownRecordFails() {
    QString error;

    QVERIFY(!service->remove(99, error));
    QVERIFY(!error.isEmpty());
}

void RecognitionHistoryServiceTests::recordIsAwaitingSyncBeforeProgressIsQueued() {
    QList<RecognitionRecord> seenByDrain;
    progress.onApply = [this, &seenByDrain] {
        // The AniList drain may run on another thread as soon as the change is queued.
        QString lookupError;
        service->findAwaitingSync(21, 6, seenByDrain, lookupError);
    };
    QString error;

    QVERIFY(service->record(session(), candidate(6), markWatched(5, 6), error));

    QCOMPARE(seenByDrain.size(), 1);
    QCOMPARE(seenByDrain.first().status(), RecognitionStatus::MarkedWatched);
}

void RecognitionHistoryServiceTests::failedQueueingRevertsRecordToUncertain() {
    progress.fail = true;
    QString error;

    const auto record = service->record(session(), candidate(6), markWatched(5, 6), error);

    QVERIFY(record.has_value());
    QCOMPARE(repository.records.first().status(), RecognitionStatus::Uncertain);
    QCOMPARE(repository.records.first().syncState(), RecognitionSyncState::NotRequired);
    QCOMPARE(repository.records.first().diagnostic(), QStringLiteral("progress-write-failed"));
}

void RecognitionHistoryServiceTests::recoveryTurnsInterruptedPendingRecordsUncertain() {
    auto interrupted = RecognitionRecord::create(session(), candidate(6), Now);
    QString error;
    QVERIFY(repository.insert(interrupted, error));

    QVERIFY2(service->recoverInterrupted(error), qPrintable(error));

    QCOMPARE(repository.records.first().status(), RecognitionStatus::Uncertain);
    QCOMPARE(repository.records.first().diagnostic(), QStringLiteral("interrupted"));
}

void RecognitionHistoryServiceTests::recoveryTurnsUnqueuedSyncPendingRecordsUncertain() {
    auto marked = RecognitionRecord::create(session(), candidate(6), Now);
    QVERIFY(marked.transitionTo(RecognitionStatus::MarkedWatched, Now));
    marked.markSyncPending(Now);
    QString error;
    QVERIFY(repository.insert(marked, error));

    QVERIFY2(service->recoverInterrupted(error), qPrintable(error));

    QCOMPARE(repository.records.first().status(), RecognitionStatus::Uncertain);
    QCOMPARE(repository.records.first().syncState(), RecognitionSyncState::NotRequired);
    QCOMPARE(repository.records.first().diagnostic(), QStringLiteral("interrupted"));
}

void RecognitionHistoryServiceTests::recoveryKeepsQueuedSyncPendingRecords() {
    auto marked = RecognitionRecord::create(session(), candidate(6), Now);
    QVERIFY(marked.transitionTo(RecognitionStatus::MarkedWatched, Now));
    marked.markSyncPending(Now);
    QString error;
    QVERIFY(repository.insert(marked, error));
    repository.queuedMediaIds = {21};

    QVERIFY2(service->recoverInterrupted(error), qPrintable(error));

    QCOMPARE(repository.records.first().status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(repository.records.first().syncState(), RecognitionSyncState::Pending);
}

QTEST_MAIN(RecognitionHistoryServiceTests)
#include "RecognitionHistoryServiceTests.moc"
