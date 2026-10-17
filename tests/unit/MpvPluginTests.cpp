#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include "../../src/application/recognition/PlaybackObservationService.h"
#include "../../src/infrastructure/recognition/QtProcessLauncher.h"
#include "../../src/infrastructure/recognition/mpv/MpvPlugin.h"

namespace {
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

class FakeTransport final : public IMpvTransport {
public:
    void open(const QString &pipeName, std::function<void(bool)> completion) override {
        openedPipes.append(pipeName);
        completion(openAttempts++ >= failuresBeforeConnect);
    }
    void write(const QByteArray &bytes) override { written.append(QJsonDocument::fromJson(bytes.trimmed()).object()); }
    void close() override { ++closes; }
    void setReadHandler(std::function<void(const QByteArray &)> handler) override { read = std::move(handler); }
    void setDisconnectedHandler(std::function<void()> handler) override { disconnected = std::move(handler); }

    void event(const QByteArray &json) { read(json + '\n'); }

    int failuresBeforeConnect = 0;
    int openAttempts = 0;
    QStringList openedPipes;
    QList<QJsonObject> written;
    int closes = 0;
    std::function<void(const QByteArray &)> read;
    std::function<void()> disconnected;
};

class FakeLauncher final : public IProcessLauncher {
public:
    bool start(const QString &startedProgram, const QStringList &startedArguments, QString &error) override {
        if (fail) {
            error = QStringLiteral("not found");
            return false;
        }
        program = startedProgram;
        arguments = startedArguments;
        return true;
    }
    [[nodiscard]] bool canStart(const QString &candidate) const override { return candidate == QStringLiteral("mpv"); }

    bool fail = false;
    QString program;
    QStringList arguments;
};

class RecordingObserver final : public IPlaybackObserver {
public:
    void onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) override {
        started.append(session);
        episodes.append(candidate.episode());
    }
    void onSessionUpdated(const PlaybackSessionId &, const PlaybackSnapshot &snapshot) override {
        snapshots.append(snapshot);
    }
    void onSessionStopped(const PlaybackSessionId &, const StopReason reason) override { stops.append(reason); }
    void onSessionDetailsChanged(const PlaybackSessionId &, const EpisodeDetails &value) override {
        details.append(value);
    }

    QList<EpisodeDetails> details;
    QList<PlaybackSession> started;
    QList<int> episodes;
    QList<PlaybackSnapshot> snapshots;
    QList<StopReason> stops;
};

/// Same routing the composition will do: plugin events into the observation policy.
class ObservationAdapter final : public IPlaybackObserver {
public:
    explicit ObservationAdapter(PlaybackObservationService &service) : service_(service) {}
    void onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) override {
        service_.start(session, candidate);
    }
    void onSessionUpdated(const PlaybackSessionId &id, const PlaybackSnapshot &snapshot) override {
        service_.update(id, snapshot);
    }
    void onSessionStopped(const PlaybackSessionId &id, const StopReason reason) override { service_.stop(id, reason); }

private:
    PlaybackObservationService &service_;
};

EpisodeCandidate candidate(const int episode = 1) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, episode, 1.0).value();
}
}

class MpvPluginTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void launchPassesIpcServerArgumentBeforeTheFile();
    void launchReturnsSessionIdentifiedByPipe();
    void launchFailureReturnsNoSession();
    void sessionStartsOnlyAfterIpcConnects();
    void sessionWithoutIpcIsNeverReported();
    void launchWithoutKnownEpisodeIsRejected();
    void observesMediaTitleAndDurationAfterConnecting();
    void timeAndPauseChangesBecomeSnapshots();
    void endFileEofStopsSessionAsEnded();
    void shutdownAndDisconnectStopSessionOnce();
    void pathChangeToAnotherFileStopsAsReplaced();
    void launchingAgainReplacesActiveSession();
    void closingBeforeThreeMinutesDoesNotComplete();
    void playingThreeMinutesCompletesOnce();
    void testConnectionChecksExecutable();
    void qtLauncherRejectsMissingProgram();
    void launchedSessionCarriesFileNameAsSourceTitle();
    void mediaTitleAndDurationBecomeDetails();
    void repeatedDetailsAreNotReported();
    void detailsAfterSessionEndAreIgnored();

private:
    MpvPluginDependencies dependencies();

    FakeTransport *transport = nullptr;
    QList<FakeTransport *> transports;
    FakeLauncher launcher;
    RecordingObserver observer;
    QDateTime now;
    int pipeCounter = 0;
};

void MpvPluginTests::init() {
    transport = nullptr;
    transports.clear();
    launcher = FakeLauncher();
    observer = RecordingObserver();
    now = Start;
    pipeCounter = 0;
}

MpvPluginDependencies MpvPluginTests::dependencies() {
    return {.transportFactory = [this] {
                auto created = std::make_unique<FakeTransport>();
                transport = created.get();
                transports.append(created.get());
                return created;
            },
            .launcher = &launcher,
            .schedule = [](int, std::function<void()> callback) { callback(); },
            .clock = [this] { return now; },
            .pipeNameFactory = [this] { return QStringLiteral("haikenanime-mpv-%1").arg(++pipeCounter); }};
}

void MpvPluginTests::launchPassesIpcServerArgumentBeforeTheFile() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    QCOMPARE(launcher.program, QStringLiteral("mpv"));
    QVERIFY(launcher.arguments.contains(QStringLiteral(R"(--input-ipc-server=\\.\pipe\haikenanime-mpv-1)")));
    QCOMPARE(launcher.arguments.last(), QStringLiteral("D:/Anime/ep01.mkv"));
    QCOMPARE(launcher.arguments.at(launcher.arguments.size() - 2), QStringLiteral("--"));
}

void MpvPluginTests::launchReturnsSessionIdentifiedByPipe() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    const auto session = plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error);

    QVERIFY(session.has_value());
    QCOMPARE(session->id.pluginId(), QStringLiteral("mpv"));
    QCOMPARE(session->id.sessionKey(), QStringLiteral("haikenanime-mpv-1"));
    QCOMPARE(session->filePath, QStringLiteral("D:/Anime/ep01.mkv"));
    QCOMPARE(session->userId, QStringLiteral("local"));
    QCOMPARE(session->startedAt, Start);
}

void MpvPluginTests::launchFailureReturnsNoSession() {
    launcher.fail = true;
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    QVERIFY(!plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    QVERIFY(!error.isEmpty());
    QVERIFY(observer.started.isEmpty());
}

void MpvPluginTests::sessionStartsOnlyAfterIpcConnects() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    QCOMPARE(transport->openedPipes, QStringList({QStringLiteral("haikenanime-mpv-1")}));
    QCOMPARE(observer.started.size(), 1);
    QCOMPARE(observer.episodes, QList<int>({1}));
}

void MpvPluginTests::sessionWithoutIpcIsNeverReported() {
    auto deps = dependencies();
    auto factory = deps.transportFactory;
    deps.transportFactory = [factory, this] {
        auto created = factory();
        static_cast<FakeTransport *>(created.get())->failuresBeforeConnect = MpvPlugin::ConnectionAttempts;
        return created;
    };
    MpvPlugin plugin(deps);
    plugin.observe(observer);
    QString error;

    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    QCOMPARE(transport->openAttempts, MpvPlugin::ConnectionAttempts);
    QVERIFY(observer.started.isEmpty());
    QVERIFY(observer.stops.isEmpty());
}

void MpvPluginTests::launchWithoutKnownEpisodeIsRejected() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    QVERIFY(!plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {}, error));

    QVERIFY(!error.isEmpty());
    QVERIFY(launcher.program.isEmpty());
}

void MpvPluginTests::observesMediaTitleAndDurationAfterConnecting() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    QStringList observed;
    for (const auto &command : transport->written) {
        const auto parts = command.value(QStringLiteral("command")).toArray();
        if (parts.at(0).toString() == QStringLiteral("observe_property")) observed.append(parts.at(2).toString());
    }
    QCOMPARE(observed, QStringList({QStringLiteral("pause"), QStringLiteral("time-pos"), QStringLiteral("path"),
                                    QStringLiteral("media-title"), QStringLiteral("duration")}));
}

void MpvPluginTests::timeAndPauseChangesBecomeSnapshots() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    now = Start.addSecs(30);
    transport->event(R"({"event":"property-change","id":2,"name":"time-pos","data":30.4})");
    now = Start.addSecs(31);
    transport->event(R"({"event":"property-change","id":1,"name":"pause","data":true})");

    QCOMPARE(observer.snapshots.size(), 2);
    QCOMPARE(observer.snapshots.at(0).position, std::chrono::seconds(30));
    QCOMPARE(observer.snapshots.at(0).state, PlaybackState::Playing);
    QCOMPARE(observer.snapshots.at(0).observedAt, Start.addSecs(30));
    QCOMPARE(observer.snapshots.at(1).state, PlaybackState::Paused);
    QVERIFY(observer.snapshots.at(1).available);
}

void MpvPluginTests::endFileEofStopsSessionAsEnded() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    transport->event(R"({"event":"end-file","reason":"eof","playlist_entry_id":1})");

    QCOMPARE(observer.stops, QList<StopReason>({StopReason::Ended}));
}

void MpvPluginTests::shutdownAndDisconnectStopSessionOnce() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    transport->event(R"({"event":"end-file","reason":"quit"})");
    transport->event(R"({"event":"shutdown"})");
    transport->disconnected();

    QCOMPARE(observer.stops, QList<StopReason>({StopReason::Closed}));
}

void MpvPluginTests::pathChangeToAnotherFileStopsAsReplaced() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    transport->event(R"({"event":"property-change","id":3,"name":"path","data":"D:/Anime/ep01.mkv"})");
    QVERIFY(observer.stops.isEmpty());
    transport->event(R"({"event":"property-change","id":3,"name":"path","data":"D:/Anime/ep02.mkv"})");

    QCOMPARE(observer.stops, QList<StopReason>({StopReason::Replaced}));
}

void MpvPluginTests::launchingAgainReplacesActiveSession() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate(1)}, error));
    auto *first = transport;

    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep02.mkv"), {.candidate = candidate(2)}, error));

    QCOMPARE(observer.stops, QList<StopReason>({StopReason::Replaced}));
    QCOMPARE(first->closes, 1);
    QCOMPARE(observer.episodes, QList<int>({1, 2}));
    first->event(R"({"event":"shutdown"})");
    QCOMPARE(observer.stops.size(), 1);
}

void MpvPluginTests::closingBeforeThreeMinutesDoesNotComplete() {
    int completed = 0;
    int incomplete = 0;
    PlaybackObservationService service(
        RecognitionPolicy(), {.completed = [&completed](auto &, auto &) { ++completed; },
                              .incomplete = [&incomplete](auto &, auto &, StopReason) { ++incomplete; }});
    ObservationAdapter adapter(service);
    MpvPlugin plugin(dependencies());
    plugin.observe(adapter);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    QList<int> seconds;
    for (int second = 10; second <= 170; second += 10) seconds.append(second);
    seconds.append(179);
    for (const int second : seconds) {
        now = Start.addSecs(second);
        transport->event(QStringLiteral(R"({"event":"property-change","id":2,"name":"time-pos","data":%1})")
                             .arg(second).toUtf8());
    }
    transport->event(R"({"event":"end-file","reason":"quit"})");

    QCOMPARE(completed, 0);
    QCOMPARE(incomplete, 1);
}

void MpvPluginTests::playingThreeMinutesCompletesOnce() {
    int completed = 0;
    PlaybackObservationService service(RecognitionPolicy(),
                                       {.completed = [&completed](auto &, auto &) { ++completed; }});
    ObservationAdapter adapter(service);
    MpvPlugin plugin(dependencies());
    plugin.observe(adapter);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    for (int second = 10; second <= 300; second += 10) {
        now = Start.addSecs(second);
        transport->event(QStringLiteral(R"({"event":"property-change","id":2,"name":"time-pos","data":%1})")
                             .arg(second).toUtf8());
    }

    QCOMPARE(completed, 1);
}

void MpvPluginTests::testConnectionChecksExecutable() {
    MpvPlugin plugin(dependencies());
    std::optional<ConnectionTestResult> result;

    plugin.testConnection([&result](const ConnectionTestResult &value) { result = value; });
    QVERIFY(result.has_value());
    QVERIFY(result->succeeded);

    plugin.setExecutable(QStringLiteral("C:/missing/mpv.exe"));
    plugin.testConnection([&result](const ConnectionTestResult &value) { result = value; });
    QVERIFY(!result->succeeded);
    QVERIFY(!result->safeMessage.isEmpty());
}

void MpvPluginTests::qtLauncherRejectsMissingProgram() {
    QtProcessLauncher launcher;
    QString error;
    const auto missing = QStringLiteral("C:/haikenanime-missing/mpv.exe");

    QVERIFY(!launcher.canStart(missing));
    QVERIFY(!launcher.start(missing, {QStringLiteral("--version")}, error));
    QVERIFY(error.contains(QStringLiteral("mpv.exe")));
    QVERIFY(!error.contains(QStringLiteral("haikenanime-missing")));
}

void MpvPluginTests::launchedSessionCarriesFileNameAsSourceTitle() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;

    const auto session = plugin.launchEpisode(QStringLiteral("D:/Anime/One Piece 01.mkv"), {.candidate = candidate()}, error);

    QCOMPARE(session->details.sourceTitle, QStringLiteral("One Piece 01.mkv"));
    QCOMPARE(observer.started.first().details.sourceTitle, QStringLiteral("One Piece 01.mkv"));
}

void MpvPluginTests::mediaTitleAndDurationBecomeDetails() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    transport->event(R"({"event":"property-change","id":4,"name":"media-title","data":"One Piece 01"})");
    transport->event(R"({"event":"property-change","id":5,"name":"duration","data":1420.6})");

    QCOMPARE(observer.details.size(), 2);
    QCOMPARE(observer.details.last().title, QStringLiteral("One Piece 01"));
    QCOMPARE(observer.details.last().durationSeconds, std::optional<int>(1420));
    QCOMPARE(observer.details.last().sourceTitle, QStringLiteral("ep01.mkv"));
}

void MpvPluginTests::repeatedDetailsAreNotReported() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));

    transport->event(R"({"event":"property-change","id":4,"name":"media-title","data":"One Piece 01"})");
    transport->event(R"({"event":"property-change","id":4,"name":"media-title","data":"One Piece 01"})");
    transport->event(R"({"event":"property-change","id":5,"name":"duration","data":null})");

    QCOMPARE(observer.details.size(), 1);
}

void MpvPluginTests::detailsAfterSessionEndAreIgnored() {
    MpvPlugin plugin(dependencies());
    plugin.observe(observer);
    QString error;
    QVERIFY(plugin.launchEpisode(QStringLiteral("D:/Anime/ep01.mkv"), {.candidate = candidate()}, error));
    transport->event(R"({"event":"end-file","reason":"quit"})");

    transport->event(R"({"event":"property-change","id":4,"name":"media-title","data":"Late"})");

    QVERIFY(observer.details.isEmpty());
}

QTEST_MAIN(MpvPluginTests)
#include "MpvPluginTests.moc"
