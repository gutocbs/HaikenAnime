#include <QSignalSpy>
#include <QtTest>

#include "../../src/presentation/recognition/NowWatchingController.h"

namespace {
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

class FakeLibrary final : public IMediaReader {
public:
    bool readAll(QList<Media> &media, QString &error) override {
        if (fail) {
            error = QStringLiteral("database is locked");
            return false;
        }
        error.clear();
        media = items;
        return true;
    }
    QList<Media> items;
    bool fail = false;
};

Media onePiece() {
    Media media;
    media.Id = 21;
    media.Name = QStringLiteral("One Piece");
    media.ConsumedChapters = 5;
    media.TotalChapters = 12;
    media.CoverLargeUrl = QStringLiteral("https://img/one-piece-large.jpg");
    media.ListStatus = UserListStatus::Current;
    return media;
}

PlaybackSession session(const QString &key, const QDateTime &startedAt, const QString &plugin = QStringLiteral("plex")) {
    return {.id = PlaybackSessionId::create(plugin, key).value(), .userId = QStringLiteral("owner"),
            .deviceId = QStringLiteral("Living Room TV"), .startedAt = startedAt,
            .details = {.sourceTitle = QStringLiteral("One Piece"), .title = QStringLiteral("Romance Dawn"),
                        .summary = QStringLiteral("Luffy sets sail."), .durationSeconds = 1440,
                        .airDate = QDate(1999, 10, 20)}};
}

EpisodeCandidate candidate(const std::optional<int> mediaId, const int episode = 6) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), mediaId, 1, episode, 0.9).value();
}
}

class NowWatchingControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void inactiveWithoutSessions();
    void associatedSessionShowsAnimeFromDatabase();
    void unassociatedSessionShowsEpisodeOnly();
    void omitsMissingEpisodeFields();
    void defaultsToMostRecentSession();
    void selectSessionSwitchesCurrent();
    void selectionMovesWhenSelectedSessionEnds();
    void mediaReadFailureFallsBackToUnassociatedAndWarns();
    void emitsChangedWhenRegistryChanges();
    void destroyedControllerStopsListeningToRegistry();

private:
    ActiveSessionRegistry registry;
    FakeLibrary library;
};

void NowWatchingControllerTests::init() {
    registry = ActiveSessionRegistry();
    library = FakeLibrary();
    library.items = {onePiece()};
}

void NowWatchingControllerTests::inactiveWithoutSessions() {
    NowWatchingController controller(registry, library);

    controller.Refresh();

    QVERIFY(!controller.active());
    QVERIFY(controller.sessions().isEmpty());
    QVERIFY(controller.current().isEmpty());
}

void NowWatchingControllerTests::associatedSessionShowsAnimeFromDatabase() {
    registry.start(session(QStringLiteral("a"), Start), candidate(21));
    NowWatchingController controller(registry, library);

    controller.Refresh();

    const auto current = controller.current();
    QVERIFY(controller.active());
    QVERIFY(current.value(QStringLiteral("associated")).toBool());
    QCOMPARE(current.value(QStringLiteral("animeTitle")).toString(), QStringLiteral("One Piece"));
    QCOMPARE(current.value(QStringLiteral("progress")).toString(), QStringLiteral("5/12"));
    QCOMPARE(current.value(QStringLiteral("coverUrl")).toString(), QStringLiteral("https://img/one-piece-large.jpg"));
    QCOMPARE(current.value(QStringLiteral("listStatus")).toString(), QStringLiteral("current"));
    QCOMPARE(current.value(QStringLiteral("season")).toInt(), 1);
    QCOMPARE(current.value(QStringLiteral("episode")).toInt(), 6);
    QCOMPARE(current.value(QStringLiteral("episodeTitle")).toString(), QStringLiteral("Romance Dawn"));
    QCOMPARE(current.value(QStringLiteral("summary")).toString(), QStringLiteral("Luffy sets sail."));
    QCOMPARE(current.value(QStringLiteral("duration")).toString(), QStringLiteral("24 min"));
    QCOMPARE(current.value(QStringLiteral("airDate")).toString(), QStringLiteral("20/10/1999"));
    QCOMPARE(current.value(QStringLiteral("source")).toString(), QStringLiteral("Plex"));
    QCOMPARE(current.value(QStringLiteral("user")).toString(), QStringLiteral("owner"));
    QCOMPARE(current.value(QStringLiteral("device")).toString(), QStringLiteral("Living Room TV"));
}

void NowWatchingControllerTests::unassociatedSessionShowsEpisodeOnly() {
    registry.start(session(QStringLiteral("a"), Start), candidate(std::nullopt));
    NowWatchingController controller(registry, library);

    controller.Refresh();

    const auto current = controller.current();
    QVERIFY(!current.value(QStringLiteral("associated")).toBool());
    QVERIFY(!current.contains(QStringLiteral("animeTitle")));
    QVERIFY(!current.contains(QStringLiteral("progress")));
    QCOMPARE(current.value(QStringLiteral("sourceTitle")).toString(), QStringLiteral("One Piece"));
    QCOMPARE(current.value(QStringLiteral("episodeTitle")).toString(), QStringLiteral("Romance Dawn"));
}

void NowWatchingControllerTests::omitsMissingEpisodeFields() {
    auto bare = session(QStringLiteral("a"), Start, QStringLiteral("mpv"));
    bare.details = {.sourceTitle = QStringLiteral("ep06.mkv")};
    registry.start(bare, candidate(21));
    NowWatchingController controller(registry, library);

    controller.Refresh();

    const auto current = controller.current();
    for (const auto *key : {"episodeTitle", "summary", "duration", "airDate"}) {
        QVERIFY2(!current.contains(QString::fromLatin1(key)), key);
    }
    QCOMPARE(current.value(QStringLiteral("source")).toString(), QStringLiteral("mpv"));
}

void NowWatchingControllerTests::defaultsToMostRecentSession() {
    registry.start(session(QStringLiteral("old"), Start), candidate(21, 1));
    registry.start(session(QStringLiteral("new"), Start.addSecs(60)), candidate(21, 2));
    NowWatchingController controller(registry, library);

    controller.Refresh();

    QCOMPARE(controller.sessions().size(), 2);
    QCOMPARE(controller.selectedSessionKey(), QStringLiteral("plex:new"));
    QCOMPARE(controller.current().value(QStringLiteral("episode")).toInt(), 2);
    QVERIFY(controller.sessions().first().toMap().value(QStringLiteral("label")).toString().contains(QStringLiteral("owner")));
}

void NowWatchingControllerTests::selectSessionSwitchesCurrent() {
    registry.start(session(QStringLiteral("old"), Start), candidate(21, 1));
    registry.start(session(QStringLiteral("new"), Start.addSecs(60)), candidate(21, 2));
    NowWatchingController controller(registry, library);
    controller.Refresh();

    controller.SelectSession(QStringLiteral("plex:old"));

    QCOMPARE(controller.selectedSessionKey(), QStringLiteral("plex:old"));
    QCOMPARE(controller.current().value(QStringLiteral("episode")).toInt(), 1);
    controller.SelectSession(QStringLiteral("plex:missing"));
    QCOMPARE(controller.selectedSessionKey(), QStringLiteral("plex:old"));
}

void NowWatchingControllerTests::selectionMovesWhenSelectedSessionEnds() {
    registry.start(session(QStringLiteral("old"), Start), candidate(21, 1));
    registry.start(session(QStringLiteral("new"), Start.addSecs(60)), candidate(21, 2));
    NowWatchingController controller(registry, library);
    controller.Refresh();

    registry.stop(session(QStringLiteral("new"), Start).id);
    controller.Refresh();
    QCOMPARE(controller.selectedSessionKey(), QStringLiteral("plex:old"));

    registry.stop(session(QStringLiteral("old"), Start).id);
    controller.Refresh();
    QVERIFY(!controller.active());
    QVERIFY(controller.selectedSessionKey().isEmpty());
    QVERIFY(controller.current().isEmpty());
}

void NowWatchingControllerTests::mediaReadFailureFallsBackToUnassociatedAndWarns() {
    library.fail = true;
    registry.start(session(QStringLiteral("a"), Start), candidate(21));
    QStringList warnings;
    NowWatchingController controller(registry, library, [&warnings](const QString &message) { warnings.append(message); });

    controller.Refresh();

    QVERIFY(!controller.current().value(QStringLiteral("associated")).toBool());
    QCOMPARE(controller.current().value(QStringLiteral("episodeTitle")).toString(), QStringLiteral("Romance Dawn"));
    QCOMPARE(warnings.size(), 1);
}

void NowWatchingControllerTests::emitsChangedWhenRegistryChanges() {
    NowWatchingController controller(registry, library);
    QSignalSpy changed(&controller, &NowWatchingController::changed);

    registry.start(session(QStringLiteral("a"), Start), candidate(21));
    QTRY_VERIFY(changed.count() >= 1);

    QVERIFY(controller.active());
}

void NowWatchingControllerTests::destroyedControllerStopsListeningToRegistry() {
    auto controller = std::make_unique<NowWatchingController>(registry, library);
    controller.reset();
    int replacement = 0;

    registry.start(session(QStringLiteral("a"), Start), candidate(21));
    QCoreApplication::processEvents();
    registry.setChangedCallback([&replacement] { ++replacement; });
    registry.stop(session(QStringLiteral("a"), Start).id);

    QCOMPARE(replacement, 1);
}

QTEST_MAIN(NowWatchingControllerTests)
#include "NowWatchingControllerTests.moc"
