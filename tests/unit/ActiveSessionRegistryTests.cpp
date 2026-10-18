#include <QtTest>

#include "../../src/application/recognition/ActiveSessionRegistry.h"

namespace {
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

PlaybackSession session(const QString &key, const QDateTime &startedAt = Start) {
    return {.id = PlaybackSessionId::create(QStringLiteral("plex"), key).value(), .userId = QStringLiteral("1"),
            .startedAt = startedAt};
}

EpisodeCandidate candidate(const int episode) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, episode, 0.9).value();
}
}

class ActiveSessionRegistryTests final : public QObject {
    Q_OBJECT

private slots:
    void startAddsSessionAndNotifies();
    void updateDetailsReplacesDetailsAndNotifies();
    void stopRemovesSessionAndNotifies();
    void keepsSeveralSessionsInStartOrder();
    void sameSessionNewEpisodeReplacesEntry();
    void ignoresDetailsForUnknownSession();
    void clearRemovesAllAndNotifiesOnce();
    void startsAsPlayingAndTracksStateChangesOnce();
};

void ActiveSessionRegistryTests::startAddsSessionAndNotifies() {
    ActiveSessionRegistry registry;
    int changes = 0;
    registry.setChangedCallback([&changes] { ++changes; });

    registry.start(session(QStringLiteral("a")), candidate(1));

    QCOMPARE(registry.sessions().size(), 1);
    QCOMPARE(registry.sessions().first().candidate.episode(), 1);
    QCOMPARE(changes, 1);
}

void ActiveSessionRegistryTests::updateDetailsReplacesDetailsAndNotifies() {
    ActiveSessionRegistry registry;
    registry.start(session(QStringLiteral("a")), candidate(1));
    int changes = 0;
    registry.setChangedCallback([&changes] { ++changes; });

    registry.updateDetails(session(QStringLiteral("a")).id, {.title = QStringLiteral("Romance Dawn")});

    QCOMPARE(registry.sessions().first().session.details.title, QStringLiteral("Romance Dawn"));
    QCOMPARE(changes, 1);
}

void ActiveSessionRegistryTests::stopRemovesSessionAndNotifies() {
    ActiveSessionRegistry registry;
    registry.start(session(QStringLiteral("a")), candidate(1));
    int changes = 0;
    registry.setChangedCallback([&changes] { ++changes; });

    registry.stop(session(QStringLiteral("a")).id);
    registry.stop(session(QStringLiteral("a")).id);

    QVERIFY(registry.sessions().isEmpty());
    QCOMPARE(changes, 1);
}

void ActiveSessionRegistryTests::keepsSeveralSessionsInStartOrder() {
    ActiveSessionRegistry registry;

    registry.start(session(QStringLiteral("late"), Start.addSecs(60)), candidate(2));
    registry.start(session(QStringLiteral("early"), Start), candidate(1));

    QCOMPARE(registry.sessions().size(), 2);
    QCOMPARE(registry.sessions().at(0).session.id.sessionKey(), QStringLiteral("early"));
    QCOMPARE(registry.sessions().at(1).session.id.sessionKey(), QStringLiteral("late"));
}

void ActiveSessionRegistryTests::sameSessionNewEpisodeReplacesEntry() {
    ActiveSessionRegistry registry;

    registry.start(session(QStringLiteral("a")), candidate(1));
    registry.start(session(QStringLiteral("a")), candidate(2));

    QCOMPARE(registry.sessions().size(), 1);
    QCOMPARE(registry.sessions().first().candidate.episode(), 2);
}

void ActiveSessionRegistryTests::ignoresDetailsForUnknownSession() {
    ActiveSessionRegistry registry;
    int changes = 0;
    registry.setChangedCallback([&changes] { ++changes; });

    registry.updateDetails(session(QStringLiteral("gone")).id, {.title = QStringLiteral("x")});

    QVERIFY(registry.sessions().isEmpty());
    QCOMPARE(changes, 0);
}

void ActiveSessionRegistryTests::clearRemovesAllAndNotifiesOnce() {
    ActiveSessionRegistry registry;
    registry.start(session(QStringLiteral("a")), candidate(1));
    registry.start(session(QStringLiteral("b")), candidate(2));
    int changes = 0;
    registry.setChangedCallback([&changes] { ++changes; });

    registry.clear();
    registry.clear();

    QVERIFY(registry.sessions().isEmpty());
    QCOMPARE(changes, 1);
}

void ActiveSessionRegistryTests::startsAsPlayingAndTracksStateChangesOnce() {
    ActiveSessionRegistry registry;
    registry.start(session(QStringLiteral("a")), candidate(1));
    int changes = 0;
    registry.setChangedCallback([&changes] { ++changes; });
    QCOMPARE(registry.sessions().first().state, PlaybackState::Playing);

    registry.updateState(session(QStringLiteral("a")).id, PlaybackState::Paused);
    registry.updateState(session(QStringLiteral("a")).id, PlaybackState::Paused);
    registry.updateState(session(QStringLiteral("gone")).id, PlaybackState::Playing);

    QCOMPARE(registry.sessions().first().state, PlaybackState::Paused);
    QCOMPARE(changes, 1);
}

QTEST_MAIN(ActiveSessionRegistryTests)
#include "ActiveSessionRegistryTests.moc"
