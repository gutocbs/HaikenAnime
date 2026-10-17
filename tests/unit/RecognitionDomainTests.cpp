#include <QtTest>

#include "../../src/domain/recognition/EpisodeCandidate.h"
#include "../../src/domain/recognition/PlaybackSession.h"
#include "../../src/domain/recognition/RecognitionPolicy.h"
#include "../../src/domain/recognition/RecognitionRecord.h"

namespace {
PlaybackSessionId sessionId(const QString &pluginId, const QString &sessionKey) {
    return PlaybackSessionId::create(pluginId, sessionKey).value();
}

EpisodeCandidate candidate(const int episode, const std::optional<int> mediaId = 21,
                           const double confidence = 1.0) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), mediaId, 1, episode, confidence).value();
}

PlaybackSession session(const QString &sessionKey = QStringLiteral("pipe-1")) {
    return PlaybackSession{.id = sessionId(QStringLiteral("mpv"), sessionKey),
                           .userId = QStringLiteral("local"),
                           .deviceId = QStringLiteral("desktop"),
                           .title = QStringLiteral("[Group] One Piece - 01.mkv"),
                           .startedAt = QDateTime(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC)};
}

QDateTime at(const int minute) {
    return QDateTime(QDate(2026, 10, 17), QTime(20, minute), QTimeZone::UTC);
}
}

class RecognitionDomainTests final : public QObject {
    Q_OBJECT

private slots:
    void policyDefaultsToThreeMinutes();
    void policyAcceptsCustomPositiveDuration();
    void policyRejectsNonPositiveDuration();
    void policyRejectsConfidenceOutsideUnitRange();
    void policyReachesThresholdOnlyAtConfiguredDuration();
    void policyTreatsCandidateBelowMinimumConfidenceAsUncertain();
    void sessionIdRequiresPluginAndSessionKey();
    void sessionIdsCompareByPluginAndSessionKey();
    void candidateRejectsEpisodeBelowOne();
    void candidateRejectsConfidenceOutsideUnitRange();
    void candidateRequiresTitleOrMediaId();
    void candidateEpisodeKeyPrefersMediaId();
    void candidateEpisodeKeyFoldsTitleCaseWhenMediaIsUnknown();
    void recordStartsPendingWithOriginalCandidate();
    void recordMovesFromPendingToEveryOutcome();
    void recordRejectsInvalidTransitions();
    void recordCorrectsAMarkedWatchedRecognition();
    void recordCorrectionKeepsOriginalCandidate();
    void recordCanBeIgnoredAfterAnOutcome();
    void recordDeduplicationKeyMatchesSameSessionAndEpisode();
    void recordDeduplicationKeyDiffersByEpisodeAndSession();
    void recordStartsWithoutIdAndSyncRequirement();
    void recordTracksSyncStateAndSafeError();
    void statusAndSyncStateNamesRoundTrip();
    void restoreRebuildsStoredRecord();
    void restoreRejectsInvalidStoredData();
    void episodeDetailsDefaultEmptyAndCompareByValue();
};

void RecognitionDomainTests::policyDefaultsToThreeMinutes() {
    const RecognitionPolicy policy;

    QCOMPARE(policy.minimumPlaybackSeconds(), 180);
}

void RecognitionDomainTests::policyAcceptsCustomPositiveDuration() {
    const auto policy = RecognitionPolicy::create(300);

    QVERIFY(policy.has_value());
    QCOMPARE(policy->minimumPlaybackSeconds(), 300);
}

void RecognitionDomainTests::policyRejectsNonPositiveDuration() {
    QVERIFY(!RecognitionPolicy::create(0).has_value());
    QVERIFY(!RecognitionPolicy::create(-1).has_value());
}

void RecognitionDomainTests::policyRejectsConfidenceOutsideUnitRange() {
    QVERIFY(!RecognitionPolicy::create(180, -0.1).has_value());
    QVERIFY(!RecognitionPolicy::create(180, 1.1).has_value());
    QVERIFY(RecognitionPolicy::create(180, 0.5).has_value());
}

void RecognitionDomainTests::policyReachesThresholdOnlyAtConfiguredDuration() {
    const RecognitionPolicy policy;

    QVERIFY(!policy.hasReachedThreshold(std::chrono::seconds(179)));
    QVERIFY(policy.hasReachedThreshold(std::chrono::seconds(180)));
}

void RecognitionDomainTests::policyTreatsCandidateBelowMinimumConfidenceAsUncertain() {
    const auto policy = RecognitionPolicy::create(180, 0.8).value();

    QVERIFY(!policy.isConfident(candidate(1, 21, 0.79)));
    QVERIFY(policy.isConfident(candidate(1, 21, 0.8)));
}

void RecognitionDomainTests::sessionIdRequiresPluginAndSessionKey() {
    QVERIFY(!PlaybackSessionId::create(QString(), QStringLiteral("1")).has_value());
    QVERIFY(!PlaybackSessionId::create(QStringLiteral("plex"), QStringLiteral("  ")).has_value());
    QVERIFY(PlaybackSessionId::create(QStringLiteral("plex"), QStringLiteral("1")).has_value());
}

void RecognitionDomainTests::sessionIdsCompareByPluginAndSessionKey() {
    QCOMPARE(sessionId(QStringLiteral("plex"), QStringLiteral("1")),
             sessionId(QStringLiteral("plex"), QStringLiteral("1")));
    QVERIFY(sessionId(QStringLiteral("plex"), QStringLiteral("1"))
            != sessionId(QStringLiteral("mpv"), QStringLiteral("1")));
    QVERIFY(sessionId(QStringLiteral("plex"), QStringLiteral("1")).key()
            != sessionId(QStringLiteral("mpv"), QStringLiteral("1")).key());
}

void RecognitionDomainTests::candidateRejectsEpisodeBelowOne() {
    QVERIFY(!EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, 0, 1.0).has_value());
}

void RecognitionDomainTests::candidateRejectsConfidenceOutsideUnitRange() {
    QVERIFY(!EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, 1, 1.5).has_value());
    QVERIFY(!EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, 1, -0.5).has_value());
}

void RecognitionDomainTests::candidateRequiresTitleOrMediaId() {
    QVERIFY(!EpisodeCandidate::create(QStringLiteral(" "), std::nullopt, 1, 1, 1.0).has_value());
    QVERIFY(EpisodeCandidate::create(QString(), 21, 1, 1, 1.0).has_value());
    QVERIFY(EpisodeCandidate::create(QStringLiteral("One Piece"), std::nullopt, std::nullopt, 1, 1.0)
                .has_value());
}

void RecognitionDomainTests::candidateEpisodeKeyPrefersMediaId() {
    const auto first = EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, 3, 1.0).value();
    const auto second = EpisodeCandidate::create(QStringLiteral("ワンピース"), 21, 1, 3, 0.9).value();

    QCOMPARE(first.episodeKey(), second.episodeKey());
}

void RecognitionDomainTests::candidateEpisodeKeyFoldsTitleCaseWhenMediaIsUnknown() {
    const auto first = EpisodeCandidate::create(QStringLiteral("One Piece"), std::nullopt, 1, 3, 1.0).value();
    const auto second = EpisodeCandidate::create(QStringLiteral(" ONE PIECE "), std::nullopt, 1, 3, 1.0).value();
    const auto otherEpisode = EpisodeCandidate::create(QStringLiteral("One Piece"), std::nullopt, 1, 4, 1.0).value();

    QCOMPARE(first.episodeKey(), second.episodeKey());
    QVERIFY(first.episodeKey() != otherEpisode.episodeKey());
}

void RecognitionDomainTests::recordStartsPendingWithOriginalCandidate() {
    const auto record = RecognitionRecord::create(session(), candidate(1), at(0));

    QCOMPARE(record.status(), RecognitionStatus::Pending);
    QCOMPARE(record.candidate().episode(), 1);
    QCOMPARE(record.effectiveCandidate().episode(), 1);
    QCOMPARE(record.createdAt(), at(0));
    QCOMPARE(record.updatedAt(), at(0));
    QCOMPARE(record.sessionId(), session().id);
    QCOMPARE(record.userId(), QStringLiteral("local"));
    QCOMPARE(record.deviceId(), QStringLiteral("desktop"));
}

void RecognitionDomainTests::recordMovesFromPendingToEveryOutcome() {
    for (const auto outcome : {RecognitionStatus::Incomplete, RecognitionStatus::MarkedWatched,
                               RecognitionStatus::AlreadyWatched, RecognitionStatus::Uncertain,
                               RecognitionStatus::Ignored}) {
        auto record = RecognitionRecord::create(session(), candidate(1), at(0));

        QVERIFY(record.transitionTo(outcome, at(3)));
        QCOMPARE(record.status(), outcome);
        QCOMPARE(record.updatedAt(), at(3));
    }
}

void RecognitionDomainTests::recordRejectsInvalidTransitions() {
    auto record = RecognitionRecord::create(session(), candidate(1), at(0));
    QVERIFY(record.transitionTo(RecognitionStatus::MarkedWatched, at(3)));

    QVERIFY(!record.transitionTo(RecognitionStatus::Pending, at(4)));
    QVERIFY(!record.transitionTo(RecognitionStatus::AlreadyWatched, at(4)));
    QCOMPARE(record.status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(record.updatedAt(), at(3));

    auto pending = RecognitionRecord::create(session(), candidate(1), at(0));
    QVERIFY(!pending.correct(candidate(2), at(4)));
    QCOMPARE(pending.status(), RecognitionStatus::Pending);
    QVERIFY(!pending.correction().has_value());
}

void RecognitionDomainTests::recordCorrectsAMarkedWatchedRecognition() {
    auto record = RecognitionRecord::create(session(), candidate(1), at(0));
    QVERIFY(record.transitionTo(RecognitionStatus::MarkedWatched, at(3)));

    QVERIFY(record.correct(candidate(2), at(4)));
    QCOMPARE(record.status(), RecognitionStatus::Corrected);
    QCOMPARE(record.effectiveCandidate().episode(), 2);
    QCOMPARE(record.updatedAt(), at(4));
}

void RecognitionDomainTests::recordCorrectionKeepsOriginalCandidate() {
    auto record = RecognitionRecord::create(session(), candidate(1), at(0));
    QVERIFY(record.transitionTo(RecognitionStatus::Uncertain, at(3)));

    QVERIFY(record.correct(candidate(2), at(5)));

    QCOMPARE(record.status(), RecognitionStatus::Corrected);
    QCOMPARE(record.candidate().episode(), 1);
    QVERIFY(record.correction().has_value());
    QCOMPARE(record.effectiveCandidate().episode(), 2);
    QCOMPARE(record.updatedAt(), at(5));
}

void RecognitionDomainTests::recordCanBeIgnoredAfterAnOutcome() {
    auto record = RecognitionRecord::create(session(), candidate(1), at(0));
    QVERIFY(record.transitionTo(RecognitionStatus::MarkedWatched, at(3)));

    QVERIFY(record.ignore(at(6)));
    QCOMPARE(record.status(), RecognitionStatus::Ignored);
    QVERIFY(record.correct(candidate(2), at(7)));
    QCOMPARE(record.status(), RecognitionStatus::Corrected);
}

void RecognitionDomainTests::recordDeduplicationKeyMatchesSameSessionAndEpisode() {
    const auto first = RecognitionRecord::create(session(), candidate(1), at(0));
    const auto repeated = RecognitionRecord::create(session(), candidate(1, 21, 0.9), at(2));

    QCOMPARE(first.deduplicationKey(), repeated.deduplicationKey());
}

void RecognitionDomainTests::recordDeduplicationKeyDiffersByEpisodeAndSession() {
    const auto first = RecognitionRecord::create(session(), candidate(1), at(0));
    const auto otherEpisode = RecognitionRecord::create(session(), candidate(2), at(0));
    const auto otherSession = RecognitionRecord::create(session(QStringLiteral("pipe-2")), candidate(1), at(0));

    QVERIFY(first.deduplicationKey() != otherEpisode.deduplicationKey());
    QVERIFY(first.deduplicationKey() != otherSession.deduplicationKey());
}

void RecognitionDomainTests::recordStartsWithoutIdAndSyncRequirement() {
    auto record = RecognitionRecord::create(session(), candidate(1), at(0));

    QCOMPARE(record.id(), 0);
    QCOMPARE(record.syncState(), RecognitionSyncState::NotRequired);
    record.assignId(42);
    QCOMPARE(record.id(), 42);
}

void RecognitionDomainTests::recordTracksSyncStateAndSafeError() {
    auto record = RecognitionRecord::create(session(), candidate(1), at(0));
    QVERIFY(record.transitionTo(RecognitionStatus::MarkedWatched, at(3)));

    record.markSyncPending(at(3));
    QCOMPARE(record.syncState(), RecognitionSyncState::Pending);
    record.markSyncFailed(QStringLiteral("rate-limit"), at(4));
    QCOMPARE(record.syncState(), RecognitionSyncState::Failed);
    QCOMPARE(record.syncError(), QStringLiteral("rate-limit"));
    QCOMPARE(record.updatedAt(), at(4));
    record.markSynced(at(5));
    QCOMPARE(record.syncState(), RecognitionSyncState::Synced);
    QVERIFY(record.syncError().isEmpty());
}

void RecognitionDomainTests::statusAndSyncStateNamesRoundTrip() {
    for (const auto status : {RecognitionStatus::Pending, RecognitionStatus::Incomplete, RecognitionStatus::MarkedWatched,
                              RecognitionStatus::AlreadyWatched, RecognitionStatus::Uncertain,
                              RecognitionStatus::Ignored, RecognitionStatus::Corrected}) {
        QCOMPARE(RecognitionStatusFromString(ToString(status)), std::optional<RecognitionStatus>(status));
    }
    for (const auto state : {RecognitionSyncState::NotRequired, RecognitionSyncState::Pending,
                             RecognitionSyncState::Failed, RecognitionSyncState::Synced}) {
        QCOMPARE(RecognitionSyncStateFromString(ToString(state)), std::optional<RecognitionSyncState>(state));
    }
    QVERIFY(!RecognitionStatusFromString(QStringLiteral("bogus")).has_value());
    QCOMPARE(ToString(RecognitionStatus::MarkedWatched), QStringLiteral("marked-watched"));
}

void RecognitionDomainTests::restoreRebuildsStoredRecord() {
    RecognitionRecordData data{.id = 7, .pluginId = QStringLiteral("plex"), .sessionKey = QStringLiteral("abc"),
                               .userId = QStringLiteral("1"), .deviceId = QStringLiteral("tv"),
                               .candidate = candidate(4), .correction = candidate(5),
                               .status = RecognitionStatus::Corrected, .diagnostic = QStringLiteral("manual"),
                               .syncState = RecognitionSyncState::Failed, .syncError = QStringLiteral("network"),
                               .createdAt = at(0), .updatedAt = at(9)};

    const auto record = RecognitionRecord::restore(data);

    QVERIFY(record.has_value());
    QCOMPARE(record->id(), 7);
    QCOMPARE(record->sessionId().key(), QStringLiteral("plex:abc"));
    QCOMPARE(record->status(), RecognitionStatus::Corrected);
    QCOMPARE(record->effectiveCandidate().episode(), 5);
    QCOMPARE(record->diagnostic(), QStringLiteral("manual"));
    QCOMPARE(record->syncState(), RecognitionSyncState::Failed);
    QCOMPARE(record->syncError(), QStringLiteral("network"));
    QCOMPARE(record->updatedAt(), at(9));
}

void RecognitionDomainTests::restoreRejectsInvalidStoredData() {
    RecognitionRecordData data{.id = 7, .pluginId = QString(), .sessionKey = QStringLiteral("abc"),
                               .candidate = candidate(4)};

    QVERIFY(!RecognitionRecord::restore(data).has_value());
}

void RecognitionDomainTests::episodeDetailsDefaultEmptyAndCompareByValue() {
    const EpisodeDetails empty;
    QVERIFY(empty.sourceTitle.isEmpty());
    QVERIFY(empty.title.isEmpty());
    QVERIFY(empty.summary.isEmpty());
    QVERIFY(!empty.durationSeconds.has_value());
    QVERIFY(!empty.airDate.has_value());

    const EpisodeDetails first{.title = QStringLiteral("Romance Dawn"), .summary = QStringLiteral("A"),
                               .durationSeconds = 1440, .airDate = QDate(1999, 10, 20)};
    auto second = first;
    QVERIFY(first == second);
    second.summary = QStringLiteral("B");
    QVERIFY(first != second);
    QVERIFY(session().details == EpisodeDetails{});
}

QTEST_MAIN(RecognitionDomainTests)
#include "RecognitionDomainTests.moc"
