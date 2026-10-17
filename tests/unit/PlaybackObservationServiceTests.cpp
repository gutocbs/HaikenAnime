#include <QtTest>

#include "../../src/application/recognition/PlaybackObservationService.h"

namespace {
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

PlaybackSession session(const QString &key = QStringLiteral("pipe-1")) {
    return PlaybackSession{.id = PlaybackSessionId::create(QStringLiteral("mpv"), key).value(),
                           .userId = QStringLiteral("local"),
                           .deviceId = QStringLiteral("desktop"),
                           .title = QStringLiteral("One Piece - 01.mkv"),
                           .startedAt = Start};
}

EpisodeCandidate candidate(const int episode = 1) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, episode, 1.0).value();
}

PlaybackSnapshot playingAt(const int seconds, const PlaybackState state = PlaybackState::Playing,
                           const bool available = true) {
    return {.position = std::chrono::seconds(seconds), .state = state, .available = available,
            .observedAt = Start.addSecs(seconds)};
}

struct Recorder {
    QList<int> completedEpisodes;
    QList<QPair<int, StopReason>> incomplete;

    ObservationCallbacks callbacks() {
        return {.completed = [this](const PlaybackSession &, const EpisodeCandidate &value) {
                    completedEpisodes.append(value.episode());
                },
                .incomplete = [this](const PlaybackSession &, const EpisodeCandidate &value, const StopReason reason) {
                    incomplete.append({value.episode(), reason});
                }};
    }
};
}

class PlaybackObservationServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void completesAfterThreeMinutesByDefault();
    void closingAtTwoMinutesFiftyNineIsIncomplete();
    void honorsConfiguredDuration();
    void policyChangeAppliesToActiveSessions();
    void pausedTimeDoesNotCount();
    void unavailableSnapshotStopsObservationAsIncomplete();
    void capsGapsBetweenObservations();
    void reportsCompletionOnlyOnce();
    void repeatedStartForSameEpisodeKeepsAccumulatedTime();
    void newEpisodeOnSameSessionReplacesPreviousObservation();
    void ignoresUpdatesForUnknownSessions();
    void stopAfterCompletionReportsNothing();

private:
    static void playUntil(PlaybackObservationService &service, const PlaybackSessionId &id, const int lastSecond,
                          const int step = 30) {
        for (int second = step; second <= lastSecond; second += step) service.update(id, playingAt(second));
    }
};

void PlaybackObservationServiceTests::completesAfterThreeMinutesByDefault() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());

    playUntil(service, session().id, 150);
    QVERIFY(recorder.completedEpisodes.isEmpty());
    service.update(session().id, playingAt(180));

    QCOMPARE(recorder.completedEpisodes, QList<int>({1}));
}

void PlaybackObservationServiceTests::closingAtTwoMinutesFiftyNineIsIncomplete() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());
    playUntil(service, session().id, 150);
    service.update(session().id, playingAt(179));

    service.stop(session().id, StopReason::Closed);

    QVERIFY(recorder.completedEpisodes.isEmpty());
    QCOMPARE(recorder.incomplete.size(), 1);
    QCOMPARE(recorder.incomplete.first().second, StopReason::Closed);
    QVERIFY(!service.isObserving(session().id));
}

void PlaybackObservationServiceTests::honorsConfiguredDuration() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy::create(60).value(), recorder.callbacks());
    service.start(session(), candidate());

    playUntil(service, session().id, 60);

    QCOMPARE(recorder.completedEpisodes, QList<int>({1}));
}

void PlaybackObservationServiceTests::policyChangeAppliesToActiveSessions() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());
    playUntil(service, session().id, 90);

    service.setPolicy(RecognitionPolicy::create(300).value());
    playUntil(service, session().id, 270);
    QVERIFY(recorder.completedEpisodes.isEmpty());
    service.update(session().id, playingAt(300));

    QCOMPARE(recorder.completedEpisodes, QList<int>({1}));
}

void PlaybackObservationServiceTests::pausedTimeDoesNotCount() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());
    playUntil(service, session().id, 120);

    service.update(session().id, playingAt(150, PlaybackState::Paused));
    for (int second = 180; second <= 600; second += 30) {
        service.update(session().id, playingAt(second, PlaybackState::Paused));
    }
    QVERIFY(recorder.completedEpisodes.isEmpty());
    service.update(session().id, playingAt(630));
    service.update(session().id, playingAt(660));

    QCOMPARE(recorder.completedEpisodes, QList<int>({1}));
}

void PlaybackObservationServiceTests::unavailableSnapshotStopsObservationAsIncomplete() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());
    playUntil(service, session().id, 120);

    service.update(session().id, playingAt(150, PlaybackState::Playing, false));
    service.update(session().id, playingAt(300));

    QVERIFY(recorder.completedEpisodes.isEmpty());
    QCOMPARE(recorder.incomplete.size(), 1);
    QCOMPARE(recorder.incomplete.first().second, StopReason::Unavailable);
}

void PlaybackObservationServiceTests::capsGapsBetweenObservations() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());

    service.update(session().id, playingAt(600));

    QVERIFY(recorder.completedEpisodes.isEmpty());
    QCOMPARE(service.accumulatedPlayback(session().id),
             PlaybackObservationService::DefaultMaximumObservationGap);
}

void PlaybackObservationServiceTests::reportsCompletionOnlyOnce() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());

    playUntil(service, session().id, 600);
    service.stop(session().id, StopReason::Ended);

    QCOMPARE(recorder.completedEpisodes, QList<int>({1}));
    QVERIFY(recorder.incomplete.isEmpty());
}

void PlaybackObservationServiceTests::repeatedStartForSameEpisodeKeepsAccumulatedTime() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());
    playUntil(service, session().id, 150);

    service.start(session(), candidate());
    service.update(session().id, playingAt(180));

    QCOMPARE(recorder.completedEpisodes, QList<int>({1}));
    QVERIFY(recorder.incomplete.isEmpty());
}

void PlaybackObservationServiceTests::newEpisodeOnSameSessionReplacesPreviousObservation() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate(1));
    playUntil(service, session().id, 90);

    service.start(session(), candidate(2));
    service.update(session().id, playingAt(120));

    QCOMPARE(recorder.incomplete.size(), 1);
    QCOMPARE(recorder.incomplete.first().first, 1);
    QCOMPARE(recorder.incomplete.first().second, StopReason::Replaced);
    QCOMPARE(service.accumulatedPlayback(session().id), std::chrono::seconds(30));
}

void PlaybackObservationServiceTests::ignoresUpdatesForUnknownSessions() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());

    service.update(session().id, playingAt(600));
    service.stop(session().id, StopReason::Closed);

    QVERIFY(recorder.completedEpisodes.isEmpty());
    QVERIFY(recorder.incomplete.isEmpty());
}

void PlaybackObservationServiceTests::stopAfterCompletionReportsNothing() {
    Recorder recorder;
    PlaybackObservationService service(RecognitionPolicy(), recorder.callbacks());
    service.start(session(), candidate());
    playUntil(service, session().id, 180);

    service.stop(session().id, StopReason::Closed);

    QCOMPARE(recorder.completedEpisodes.size(), 1);
    QVERIFY(recorder.incomplete.isEmpty());
    QVERIFY(!service.isObserving(session().id));
}

QTEST_MAIN(PlaybackObservationServiceTests)
#include "PlaybackObservationServiceTests.moc"
