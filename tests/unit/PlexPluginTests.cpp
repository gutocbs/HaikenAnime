#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include "../../src/infrastructure/recognition/plex/PlexPlugin.h"
#include "PlexTestSupport.h"

namespace {
const auto ServerUri = QStringLiteral("https://10-0-0-20.server-abc.plex.direct:32400");
const auto SessionsKey = QStringLiteral("GET https://10-0-0-20.server-abc.plex.direct:32400/status/sessions");
const auto ResourcesKey = QStringLiteral("GET https://clients.plex.tv/api/v2/resources");
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

PlexServer livingRoom() {
    return {.name = QStringLiteral("Living Room PMS"),
            .clientIdentifier = QStringLiteral("server-abc"),
            .accessToken = QStringLiteral("server-access-token-xyz"),
            .owned = true,
            .connections = {{.uri = QUrl(ServerUri), .local = true}}};
}

QByteArray sessionsBody(const QList<QJsonObject> &items) {
    QJsonArray metadata;
    for (const auto &item : items) metadata.append(item);
    return QJsonDocument(QJsonObject{{QStringLiteral("MediaContainer"),
                                      QJsonObject{{QStringLiteral("size"), metadata.size()},
                                                  {QStringLiteral("Metadata"), metadata}}}})
        .toJson(QJsonDocument::Compact);
}

QJsonObject episode(const QString &sessionKey, const QString &userId, const int number, const int offsetSeconds,
                    const QString &state = QStringLiteral("playing")) {
    return {{QStringLiteral("type"), QStringLiteral("episode")},
            {QStringLiteral("sessionKey"), sessionKey},
            {QStringLiteral("grandparentTitle"), QStringLiteral("One Piece")},
            {QStringLiteral("title"), QStringLiteral("Episode %1").arg(number)},
            {QStringLiteral("parentIndex"), 1},
            {QStringLiteral("index"), number},
            {QStringLiteral("viewOffset"), offsetSeconds * 1000},
            {QStringLiteral("User"), QJsonObject{{QStringLiteral("id"), userId}, {QStringLiteral("title"), userId}}},
            {QStringLiteral("Player"), QJsonObject{{QStringLiteral("machineIdentifier"), QStringLiteral("tv-") + userId},
                                                   {QStringLiteral("state"), state}}}};
}

class RecordingObserver final : public IPlaybackObserver {
public:
    void onSessionStarted(const PlaybackSession &session, const EpisodeCandidate &candidate) override {
        started.append(session);
        candidates.append(candidate);
    }
    void onSessionUpdated(const PlaybackSessionId &id, const PlaybackSnapshot &snapshot) override {
        updates.append({id.key(), snapshot});
    }
    void onSessionStopped(const PlaybackSessionId &id, const StopReason reason) override {
        stops.append({id.key(), reason});
    }

    QList<PlaybackSession> started;
    QList<EpisodeCandidate> candidates;
    QList<QPair<QString, PlaybackSnapshot>> updates;
    QList<QPair<QString, StopReason>> stops;
};
}

class PlexPluginTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void reportsOnlySelectedUsers();
    void preservesUserAndDeviceIdentityPerSession();
    void normalizesEpisodeCandidateFromMetadata();
    void repeatedPollsDoNotStartTheSameSessionTwice();
    void nextEpisodeInSameSessionStartsAgain();
    void pausedAndBufferingDoNotCountAsPlaying();
    void vanishedSessionStopsAsClosed();
    void deselectingUserStopsTheirSession();
    void unavailableServerStopsSessionsAndKeepsPolling();
    void unauthorizedServerStopsPollingSafely();
    void noServerOrNoSelectedUsersMakesNoRequest();
    void observeSchedulesPollingAtConfiguredInterval();
    void stopCancelsPolling();
    void selectServerRediscoversWithAccountToken();
    void testConnectionReportsMissingLogin();
    void serverTokenNeverAppearsInLogs();

private:
    PlexPluginDependencies dependencies(std::function<void(int, std::function<void()>)> schedule = {});

    FakePlexHttp http;
    FakeSecretStore secrets;
    std::unique_ptr<PlexApiClient> api;
    std::unique_ptr<PlexAuthentication> authentication;
    RecordingObserver observer;
    QDateTime now;
};

void PlexPluginTests::init() {
    http = FakePlexHttp();
    secrets = FakeSecretStore();
    api = std::make_unique<PlexApiClient>(http, PlexClientIdentity{.clientIdentifier = QStringLiteral("client-123")});
    authentication = std::make_unique<PlexAuthentication>(PlexAuthenticationDependencies{
        .http = &http, .secrets = &secrets, .identity = {.clientIdentifier = QStringLiteral("client-123")}});
    observer = RecordingObserver();
    now = Start;
}

PlexPluginDependencies PlexPluginTests::dependencies(std::function<void(int, std::function<void()>)> schedule) {
    return {.api = api.get(),
            .authentication = authentication.get(),
            .schedule = schedule ? std::move(schedule) : [](int, std::function<void()>) {},
            .clock = [this] { return now; }};
}

void PlexPluginTests::reportsOnlySelectedUsers() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60),
                                                                  episode(QStringLiteral("13"), QStringLiteral("7"), 5, 30)})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});

    plugin.observe(observer);

    QCOMPARE(observer.started.size(), 1);
    QCOMPARE(observer.started.first().userId, QStringLiteral("1"));
    QCOMPARE(observer.updates.size(), 1);
}

void PlexPluginTests::preservesUserAndDeviceIdentityPerSession() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60),
                                                                  episode(QStringLiteral("13"), QStringLiteral("7"), 5, 30)})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1"), QStringLiteral("7")});

    plugin.observe(observer);

    QCOMPARE(observer.started.size(), 2);
    QCOMPARE(observer.started.at(0).id.key(), QStringLiteral("plex:server-abc:12"));
    QCOMPARE(observer.started.at(0).deviceId, QStringLiteral("tv-1"));
    QCOMPARE(observer.started.at(1).id.key(), QStringLiteral("plex:server-abc:13"));
    QCOMPARE(observer.started.at(1).userId, QStringLiteral("7"));
    QCOMPARE(observer.started.at(1).deviceId, QStringLiteral("tv-7"));
}

void PlexPluginTests::normalizesEpisodeCandidateFromMetadata() {
    http.route(SessionsKey, {.status = 200, .body = PlexFixture(QStringLiteral("plex-sessions.json"))});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});

    plugin.observe(observer);

    QCOMPARE(observer.candidates.size(), 1);
    const auto &candidate = observer.candidates.first();
    QCOMPARE(candidate.title(), QStringLiteral("One Piece"));
    QCOMPARE(candidate.season(), std::optional<int>(1));
    QCOMPARE(candidate.episode(), 1);
    QVERIFY(!candidate.mediaId().has_value());
    QCOMPARE(candidate.confidence(), PlexPlugin::StructuredMetadataConfidence);
    QCOMPARE(observer.updates.first().second.position, std::chrono::seconds(61));
    QCOMPARE(observer.updates.first().second.observedAt, Start);
}

void PlexPluginTests::repeatedPollsDoNotStartTheSameSessionTwice() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60)})});
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 75)})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});

    plugin.observe(observer);
    now = Start.addSecs(15);
    plugin.pollSessions();
    plugin.pollSessions();

    QCOMPARE(observer.started.size(), 1);
    QCOMPARE(observer.updates.size(), 3);
    QVERIFY(observer.stops.isEmpty());
}

void PlexPluginTests::nextEpisodeInSameSessionStartsAgain() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 1400)})});
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 2, 5)})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});
    plugin.observe(observer);

    plugin.pollSessions();
    plugin.pollSessions();

    QCOMPARE(observer.started.size(), 2);
    QCOMPARE(observer.candidates.at(1).episode(), 2);
    QVERIFY(observer.stops.isEmpty());
}

void PlexPluginTests::pausedAndBufferingDoNotCountAsPlaying() {
    http.route(SessionsKey, {.status = 200,
                             .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60, QStringLiteral("paused")),
                                                   episode(QStringLiteral("13"), QStringLiteral("1"), 2, 60, QStringLiteral("buffering")),
                                                   episode(QStringLiteral("14"), QStringLiteral("1"), 3, 60)})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});

    plugin.observe(observer);

    QCOMPARE(observer.updates.size(), 3);
    QCOMPARE(observer.updates.at(0).second.state, PlaybackState::Paused);
    QCOMPARE(observer.updates.at(1).second.state, PlaybackState::Paused);
    QCOMPARE(observer.updates.at(2).second.state, PlaybackState::Playing);
}

void PlexPluginTests::vanishedSessionStopsAsClosed() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60)})});
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});
    plugin.observe(observer);

    plugin.pollSessions();
    plugin.pollSessions();
    plugin.pollSessions();

    QCOMPARE(observer.stops.size(), 1);
    QCOMPARE(observer.stops.first().first, QStringLiteral("plex:server-abc:12"));
    QCOMPARE(observer.stops.first().second, StopReason::Closed);
}

void PlexPluginTests::deselectingUserStopsTheirSession() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60)})});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});
    plugin.observe(observer);
    plugin.pollSessions();

    plugin.setSelectedUsers({QStringLiteral("7")});
    plugin.pollSessions();

    QCOMPARE(observer.stops.size(), 1);
    QCOMPARE(observer.started.size(), 1);
}

void PlexPluginTests::unavailableServerStopsSessionsAndKeepsPolling() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60)})});
    http.route(SessionsKey, {.status = 0});
    int scheduledPolls = 0;
    std::function<void()> next;
    PlexPlugin plugin(dependencies([&](int, std::function<void()> callback) {
        ++scheduledPolls;
        next = std::move(callback);
    }));
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});

    plugin.observe(observer);
    auto poll = std::exchange(next, {});
    poll();

    QCOMPARE(observer.stops.size(), 1);
    QCOMPARE(observer.stops.first().second, StopReason::Unavailable);
    QVERIFY(!plugin.lastError().isEmpty());
    QVERIFY(next);
    QCOMPARE(scheduledPolls, 2);
}

void PlexPluginTests::unauthorizedServerStopsPollingSafely() {
    http.route(SessionsKey, {.status = 401});
    int scheduledPolls = 0;
    PlexPlugin plugin(dependencies([&scheduledPolls](int, std::function<void()>) { ++scheduledPolls; }));
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});

    plugin.observe(observer);

    QVERIFY(!plugin.lastError().isEmpty());
    QVERIFY(!plugin.lastError().contains(QStringLiteral("server-access-token-xyz")));
    QCOMPARE(scheduledPolls, 0);
}

void PlexPluginTests::noServerOrNoSelectedUsersMakesNoRequest() {
    PlexPlugin plugin(dependencies());
    plugin.observe(observer);
    plugin.pollSessions();
    plugin.setServer(livingRoom());
    plugin.pollSessions();

    QVERIFY(http.requests.isEmpty());
}

void PlexPluginTests::observeSchedulesPollingAtConfiguredInterval() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({})});
    QList<int> delays;
    PlexPlugin plugin(dependencies([&delays](const int delay, std::function<void()>) { delays.append(delay); }));
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});
    plugin.setPollInterval(30);

    plugin.observe(observer);

    QCOMPARE(http.requests.size(), 1);
    QCOMPARE(delays, QList<int>({30000}));
}

void PlexPluginTests::stopCancelsPolling() {
    http.route(SessionsKey, {.status = 200, .body = sessionsBody({episode(QStringLiteral("12"), QStringLiteral("1"), 1, 60)})});
    std::function<void()> next;
    PlexPlugin plugin(dependencies([&next](int, std::function<void()> callback) { next = std::move(callback); }));
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1")});
    plugin.observe(observer);

    plugin.stop();
    const auto requests = http.requests.size();
    if (next) next();

    QCOMPARE(http.requests.size(), requests);
    QCOMPARE(observer.stops.size(), 1);
    QCOMPARE(observer.stops.first().second, StopReason::Closed);
}

void PlexPluginTests::selectServerRediscoversWithAccountToken() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = QStringLiteral("account-token");
    http.route(ResourcesKey, {.status = 200, .body = PlexFixture(QStringLiteral("plex-resources.json"))});
    PlexPlugin plugin(dependencies());
    std::optional<bool> selected;

    plugin.selectServer(QStringLiteral("server-abc"), [&selected](const bool value, const QString &) { selected = value; });

    QVERIFY(selected.value_or(false));
    QCOMPARE(plugin.server()->clientIdentifier, QStringLiteral("server-abc"));
    QCOMPARE(http.header(http.requests.first(), "X-Plex-Token"), QByteArray("account-token"));

    plugin.selectServer(QStringLiteral("missing"), [&selected](const bool value, const QString &) { selected = value; });
    QVERIFY(!selected.value_or(true));
}

void PlexPluginTests::testConnectionReportsMissingLogin() {
    PlexPlugin plugin(dependencies());
    std::optional<ConnectionTestResult> result;

    plugin.testConnection([&result](const ConnectionTestResult &value) { result = value; });

    QVERIFY(result.has_value());
    QVERIFY(!result->succeeded);
    QVERIFY(!result->safeMessage.isEmpty());
}

void PlexPluginTests::serverTokenNeverAppearsInLogs() {
    LogCapture logs;
    http.route(SessionsKey, {.status = 200, .body = PlexFixture(QStringLiteral("plex-sessions.json"))});
    http.route(SessionsKey, {.status = 401});
    PlexPlugin plugin(dependencies());
    plugin.setServer(livingRoom());
    plugin.setSelectedUsers({QStringLiteral("1"), QStringLiteral("7")});
    plugin.observe(observer);
    plugin.pollSessions();
    plugin.pollSessions();

    QVERIFY(!logs.contains(QStringLiteral("server-access-token-xyz")));
    QVERIFY(!plugin.lastError().contains(QStringLiteral("server-access-token-xyz")));
}

QTEST_MAIN(PlexPluginTests)
#include "PlexPluginTests.moc"
