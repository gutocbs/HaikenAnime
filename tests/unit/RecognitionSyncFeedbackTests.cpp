#include <QtTest>

#include "../../src/application/recognition/RecognitionHistoryService.h"
#include "../../src/application/recognition/RecognitionSyncFeedback.h"

namespace {
const QDateTime Now(QDate(2026, 10, 17), QTime(21, 0), QTimeZone::UTC);
constexpr int MediaId = 21;

bool IsAwaitingSync(const RecognitionRecord &record) {
    return record.syncState() == RecognitionSyncState::Pending || record.syncState() == RecognitionSyncState::Failed;
}

class FakeHistoryRepository final : public IRecognitionHistoryRepository {
public:
    bool insert(RecognitionRecord &record, QString &error) override {
        error.clear();
        record.assignId(++lastId);
        records.append(record);
        return true;
    }

    bool update(const RecognitionRecord &record, QString &error) override {
        error.clear();
        if (failUpdates) {
            error = QStringLiteral("database is locked");
            return false;
        }
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
        found.clear();
        if (failFind) {
            error = QStringLiteral("database is locked");
            return false;
        }
        for (auto record = records.crbegin(); record != records.crend(); ++record) {
            const auto &effective = record->effectiveCandidate();
            if (query.recordId && record->id() != *query.recordId) continue;
            if (!query.deduplicationKey.isEmpty() && record->deduplicationKey() != query.deduplicationKey) continue;
            if (query.mediaId && effective.mediaId() != query.mediaId) continue;
            if (query.maxEpisode && effective.episode() > *query.maxEpisode) continue;
            if (query.awaitingSync && !IsAwaitingSync(*record)) continue;
            if (found.size() >= query.limit) break;
            found.append(*record);
        }
        return true;
    }

    [[nodiscard]] const RecognitionRecord &byId(const qint64 id) const {
        for (const auto &record : records) {
            if (record.id() == id) return record;
        }
        return records.first();
    }

    bool remove(const qint64 recordId, const std::optional<QueuedProgress> &, QString &error) override {
        error.clear();
        return records.removeIf([recordId](const RecognitionRecord &record) { return record.id() == recordId; }) > 0;
    }

    QList<RecognitionRecord> records;
    qint64 lastId = 0;
    bool failFind = false;
    bool failUpdates = false;
};

class FakeProgressWriter final : public IRecognitionProgressWriter {
public:
    bool apply(const Media &, const Media &, QString &error) override {
        error.clear();
        return true;
    }
};

PlaybackSession session(const QString &key) {
    return {.id = PlaybackSessionId::create(QStringLiteral("plex"), key).value(), .userId = QStringLiteral("7"),
            .deviceId = QStringLiteral("tv"), .title = QStringLiteral("One Piece"), .startedAt = Now.addSecs(-300)};
}

EpisodeCandidate candidate(const int episode, const std::optional<int> mediaId = MediaId) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), mediaId, 1, episode, 1.0).value();
}

Media media(const int consumed) {
    Media value;
    value.Id = MediaId;
    value.ConsumedChapters = consumed;
    value.TotalChapters = 24;
    value.ListStatus = UserListStatus::Current;
    return value;
}

ProgressDecision markWatched(const int from, const int to) {
    return {.kind = ProgressDecisionKind::MarkWatched, .previous = media(from), .updated = media(to)};
}

AniListPendingChange progressChange(const int progress, const int mediaId = MediaId) {
    AniListPendingChange change;
    change.id = 99;
    change.mediaId = mediaId;
    change.field = AniListField::Progress;
    change.newValue = progress;
    change.status = AniListPendingChangeStatus::Succeeded;
    return change;
}
}

class RecognitionSyncFeedbackTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void sentProgressMarksMatchingRecordSynced();
    void sentProgressAlsoSettlesEarlierEpisodesCompactedIntoIt();
    void retryableFailureMarksRecordFailedWithRetryMessage();
    void definitiveFailureMarksRecordFailedWithAccountMessage();
    void failureMessageNeverContainsRawErrorOrToken();
    void laterSuccessClearsPreviousFailure();
    void correctedRecordIsMatchedByItsCorrection();
    void ignoresOtherMediaLaterEpisodesAndNonProgressFields();
    void ignoresRecordsThatNeverRequiredSync();
    void historyFailureIsLoggedWithoutThrowing();

private:
    std::unique_ptr<FakeHistoryRepository> repository;
    std::unique_ptr<FakeProgressWriter> progress;
    std::unique_ptr<RecognitionHistoryService> history;
    QStringList logs;

    qint64 recordWatched(const QString &sessionKey, int episode);
};

void RecognitionSyncFeedbackTests::init() {
    repository = std::make_unique<FakeHistoryRepository>();
    progress = std::make_unique<FakeProgressWriter>();
    history = std::make_unique<RecognitionHistoryService>(*repository, *progress, [] { return Now; });
    logs.clear();
}

qint64 RecognitionSyncFeedbackTests::recordWatched(const QString &sessionKey, const int episode) {
    QString error;
    const auto record = history->record(session(sessionKey), candidate(episode), markWatched(episode - 1, episode),
                                        error);
    if (!record.has_value()) qFatal("recording failed: %s", qPrintable(error));
    return record->id();
}

void RecognitionSyncFeedbackTests::sentProgressMarksMatchingRecordSynced() {
    // Arrange
    const auto recordId = recordWatched(QStringLiteral("a"), 5);
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesSent({progressChange(5)});

    // Assert
    QCOMPARE(repository->byId(recordId).syncState(), RecognitionSyncState::Synced);
    QVERIFY(repository->byId(recordId).syncError().isEmpty());
}

void RecognitionSyncFeedbackTests::sentProgressAlsoSettlesEarlierEpisodesCompactedIntoIt() {
    // Arrange: episodes 4 and 5 were watched before a drain; the outbox compacts them into progress 5.
    const auto earlier = recordWatched(QStringLiteral("a"), 4);
    const auto later = recordWatched(QStringLiteral("b"), 5);
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesSent({progressChange(5)});

    // Assert
    QCOMPARE(repository->byId(earlier).syncState(), RecognitionSyncState::Synced);
    QCOMPARE(repository->byId(later).syncState(), RecognitionSyncState::Synced);
}

void RecognitionSyncFeedbackTests::retryableFailureMarksRecordFailedWithRetryMessage() {
    // Arrange
    const auto recordId = recordWatched(QStringLiteral("a"), 5);
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesFailed({progressChange(5)}, AniListSyncErrorCategory::Network);

    // Assert
    const auto &record = repository->byId(recordId);
    QCOMPARE(record.syncState(), RecognitionSyncState::Failed);
    QCOMPARE(record.syncError(), RecognitionSyncFeedback::SafeError(AniListSyncErrorCategory::Network));
    QCOMPARE(record.syncError(), QStringLiteral("falha temporária de comunicação; nova tentativa automática"));
    QCOMPARE(record.status(), RecognitionStatus::MarkedWatched);
}

void RecognitionSyncFeedbackTests::definitiveFailureMarksRecordFailedWithAccountMessage() {
    // Arrange
    const auto recordId = recordWatched(QStringLiteral("a"), 5);
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesFailed({progressChange(5)}, AniListSyncErrorCategory::Authentication);

    // Assert
    const auto &record = repository->byId(recordId);
    QCOMPARE(record.syncState(), RecognitionSyncState::Failed);
    QCOMPARE(record.syncError(), QStringLiteral("conta do AniList não autorizada; reconecte a conta"));
    QCOMPARE(record.status(), RecognitionStatus::MarkedWatched);
}

void RecognitionSyncFeedbackTests::failureMessageNeverContainsRawErrorOrToken() {
    // Arrange
    const QList categories{AniListSyncErrorCategory::Authentication, AniListSyncErrorCategory::Authorization,
                           AniListSyncErrorCategory::RateLimit,      AniListSyncErrorCategory::GraphQl,
                           AniListSyncErrorCategory::Network,        AniListSyncErrorCategory::Timeout,
                           AniListSyncErrorCategory::Persistence,    AniListSyncErrorCategory::InvalidData,
                           AniListSyncErrorCategory::Unknown};

    for (const auto category : categories) {
        // Act
        const auto message = RecognitionSyncFeedback::SafeError(category);

        // Assert
        QVERIFY(!message.isEmpty());
        QVERIFY(!message.contains(QStringLiteral("Bearer"), Qt::CaseInsensitive));
        QVERIFY(!message.contains(QStringLiteral("token"), Qt::CaseInsensitive));
        QCOMPARE(message.contains(QStringLiteral("nova tentativa")), IsRetryable(category));
    }
}

void RecognitionSyncFeedbackTests::laterSuccessClearsPreviousFailure() {
    // Arrange
    const auto recordId = recordWatched(QStringLiteral("a"), 5);
    RecognitionSyncFeedback feedback(*history);
    feedback.changesFailed({progressChange(5)}, AniListSyncErrorCategory::Timeout);

    // Act
    feedback.changesSent({progressChange(5)});

    // Assert
    QCOMPARE(repository->byId(recordId).syncState(), RecognitionSyncState::Synced);
    QVERIFY(repository->byId(recordId).syncError().isEmpty());
}

void RecognitionSyncFeedbackTests::correctedRecordIsMatchedByItsCorrection() {
    // Arrange: recognized as media 21 ep 5 without progress, then corrected to media 42 ep 2.
    QString error;
    const auto uncertain = history->record(session(QStringLiteral("a")), candidate(5),
                                           {.kind = ProgressDecisionKind::Uncertain, .reason = QStringLiteral("x")},
                                           error);
    QVERIFY(uncertain.has_value());
    auto corrected = media(2);
    corrected.Id = 42;
    auto before = media(1);
    before.Id = 42;
    QVERIFY2(history->correct(uncertain->id(), candidate(2, 42),
                              {.kind = ProgressDecisionKind::MarkWatched, .previous = before, .updated = corrected},
                              error),
             qPrintable(error));
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesSent({progressChange(5, MediaId)});
    const auto afterOriginalMedia = repository->byId(uncertain->id()).syncState();
    feedback.changesSent({progressChange(2, 42)});

    // Assert
    QCOMPARE(afterOriginalMedia, RecognitionSyncState::Pending);
    QCOMPARE(repository->byId(uncertain->id()).status(), RecognitionStatus::Corrected);
    QCOMPARE(repository->byId(uncertain->id()).syncState(), RecognitionSyncState::Synced);
}

void RecognitionSyncFeedbackTests::ignoresOtherMediaLaterEpisodesAndNonProgressFields() {
    // Arrange
    const auto recordId = recordWatched(QStringLiteral("a"), 5);
    auto score = progressChange(5);
    score.field = AniListField::PersonalScore;
    auto textual = progressChange(5);
    textual.newValue = QStringLiteral("5");
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesSent({progressChange(5, 999), progressChange(4), score, textual});
    feedback.changesFailed({progressChange(5, 999), score}, AniListSyncErrorCategory::Network);

    // Assert
    QCOMPARE(repository->byId(recordId).syncState(), RecognitionSyncState::Pending);
}

void RecognitionSyncFeedbackTests::ignoresRecordsThatNeverRequiredSync() {
    // Arrange
    QString error;
    const auto alreadyWatched = history->record(session(QStringLiteral("a")), candidate(5),
                                                {.kind = ProgressDecisionKind::AlreadyWatched}, error);
    QVERIFY(alreadyWatched.has_value());
    RecognitionSyncFeedback feedback(*history);

    // Act
    feedback.changesSent({progressChange(5)});
    feedback.changesFailed({progressChange(5)}, AniListSyncErrorCategory::Network);

    // Assert
    QCOMPARE(repository->byId(alreadyWatched->id()).syncState(), RecognitionSyncState::NotRequired);
}

void RecognitionSyncFeedbackTests::historyFailureIsLoggedWithoutThrowing() {
    // Arrange
    recordWatched(QStringLiteral("a"), 5);
    repository->failUpdates = true;
    RecognitionSyncFeedback feedback(*history, [this](const QString &message) { logs.append(message); });

    // Act
    feedback.changesSent({progressChange(5)});
    repository->failUpdates = false;
    repository->failFind = true;
    feedback.changesFailed({progressChange(5)}, AniListSyncErrorCategory::Network);

    // Assert
    QCOMPARE(logs.size(), 2);
    QVERIFY(logs.first().contains(QStringLiteral("media 21")));
    QVERIFY(logs.last().contains(QStringLiteral("media 21")));
}

QTEST_MAIN(RecognitionSyncFeedbackTests)
#include "RecognitionSyncFeedbackTests.moc"
