#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QtTest>

#include "../../src/infrastructure/recognition/plex/PlexApiClient.h"
#include "PlexTestSupport.h"

namespace {
const auto ResourcesUrl = QStringLiteral("GET https://clients.plex.tv/api/v2/resources");
const auto ServerUri = QStringLiteral("https://10-0-0-20.server-abc.plex.direct:32400");

PlexServer livingRoom() {
    return {.name = QStringLiteral("Living Room PMS"),
            .clientIdentifier = QStringLiteral("server-abc"),
            .accessToken = QStringLiteral("server-access-token-xyz"),
            .owned = true,
            .connections = {{.uri = QUrl(ServerUri), .local = true}}};
}
}

class PlexApiClientTests final : public QObject {
    Q_OBJECT

private slots:
    void discoverServersSendsTokenAndKeepsOnlyServers();
    void preferredUriIsLocalThenRemoteThenRelay();
    void discoverServersReportsUnauthorized();
    void listSessionsParsesEpisodesAndIdentity();
    void listSessionsSkipsEntriesWithoutSessionKey();
    void listSessionsAcceptsEmptyContainer();
    void listSessionsReportsUnavailableAndInvalidResponses();
    void listUsersSkipsSystemAccount();
    void qtHttpTimesOutWithoutResponse();
    void qtHttpReturnsStatusAndBody();
};

void PlexApiClientTests::discoverServersSendsTokenAndKeepsOnlyServers() {
    FakePlexHttp http;
    http.route(ResourcesUrl, {.status = 200, .body = PlexFixture(QStringLiteral("plex-resources.json"))});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    PlexApiError error = PlexApiError::InvalidResponse;
    QList<PlexServer> servers;

    client.discoverServers(QStringLiteral("account-token"), [&](const PlexApiError result, const QList<PlexServer> &found) {
        error = result;
        servers = found;
    });

    QCOMPARE(error, PlexApiError::None);
    QCOMPARE(servers.size(), 2);
    QCOMPARE(servers.first().clientIdentifier, QStringLiteral("server-abc"));
    QCOMPARE(servers.first().accessToken, QStringLiteral("server-access-token-xyz"));
    QVERIFY(servers.first().owned);
    QCOMPARE(servers.first().connections.size(), 3);
    const auto &request = http.requests.first();
    QCOMPARE(http.header(request, "X-Plex-Token"), QByteArray("account-token"));
    QCOMPARE(http.header(request, "X-Plex-Client-Identifier"), QByteArray("client-123"));
    QCOMPARE(QUrlQuery(request.url).queryItemValue(QStringLiteral("includeHttps")), QStringLiteral("1"));
    QCOMPARE(QUrlQuery(request.url).queryItemValue(QStringLiteral("includeRelay")), QStringLiteral("1"));
}

void PlexApiClientTests::preferredUriIsLocalThenRemoteThenRelay() {
    FakePlexHttp http;
    http.route(ResourcesUrl, {.status = 200, .body = PlexFixture(QStringLiteral("plex-resources.json"))});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    QList<PlexServer> servers;
    client.discoverServers(QStringLiteral("t"), [&](PlexApiError, const QList<PlexServer> &found) { servers = found; });

    QCOMPARE(servers.at(0).preferredUri(), std::optional<QUrl>(QUrl(ServerUri)));
    QCOMPARE(servers.at(1).preferredUri(), std::optional<QUrl>(QUrl(QStringLiteral("https://relay.server-friend.plex.direct:8443"))));
    PlexServer remoteOnly{.connections = {{.uri = QUrl(QStringLiteral("https://relay.x:8443")), .relay = true},
                                          {.uri = QUrl(QStringLiteral("https://remote.x:32400"))}}};
    QCOMPARE(remoteOnly.preferredUri(), std::optional<QUrl>(QUrl(QStringLiteral("https://remote.x:32400"))));
    QVERIFY(!PlexServer{}.preferredUri().has_value());
}

void PlexApiClientTests::discoverServersReportsUnauthorized() {
    FakePlexHttp http;
    http.route(ResourcesUrl, {.status = 401});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    PlexApiError error = PlexApiError::None;

    client.discoverServers(QStringLiteral("expired"), [&](const PlexApiError result, const QList<PlexServer> &) {
        error = result;
    });

    QCOMPARE(error, PlexApiError::Unauthorized);
}

void PlexApiClientTests::listSessionsParsesEpisodesAndIdentity() {
    FakePlexHttp http;
    http.route(QStringLiteral("GET ") + ServerUri + QStringLiteral("/status/sessions"),
               {.status = 200, .body = PlexFixture(QStringLiteral("plex-sessions.json"))});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    QList<PlexSession> sessions;
    PlexApiError error = PlexApiError::InvalidResponse;

    client.listSessions(livingRoom(), [&](const PlexApiError result, const QList<PlexSession> &found) {
        error = result;
        sessions = found;
    });

    QCOMPARE(error, PlexApiError::None);
    QCOMPARE(sessions.size(), 3);
    const auto &first = sessions.at(0);
    QCOMPARE(first.sessionKey, QStringLiteral("12"));
    QCOMPARE(first.type, QStringLiteral("episode"));
    QCOMPARE(first.showTitle, QStringLiteral("One Piece"));
    QCOMPARE(first.episodeTitle, QStringLiteral("Romance Dawn"));
    QCOMPARE(first.season, std::optional<int>(1));
    QCOMPARE(first.episode, std::optional<int>(1));
    QCOMPARE(first.viewOffsetMs, 61500);
    QCOMPARE(first.durationMs, 1440000);
    QCOMPARE(first.userId, QStringLiteral("1"));
    QCOMPARE(first.userName, QStringLiteral("owner"));
    QCOMPARE(first.playerId, QStringLiteral("tv-1"));
    QCOMPARE(first.state, QStringLiteral("playing"));
    QCOMPARE(sessions.at(1).userId, QStringLiteral("7"));
    QCOMPARE(sessions.at(1).state, QStringLiteral("paused"));
    QCOMPARE(sessions.at(1).durationMs, 0);
    QCOMPARE(sessions.at(2).type, QStringLiteral("movie"));
    QVERIFY(!sessions.at(2).episode.has_value());
    const auto &request = http.requests.first();
    QCOMPARE(http.header(request, "X-Plex-Token"), QByteArray("server-access-token-xyz"));
    QCOMPARE(http.header(request, "Accept"), QByteArray("application/json"));
}

void PlexApiClientTests::listSessionsSkipsEntriesWithoutSessionKey() {
    FakePlexHttp http;
    http.route(QStringLiteral("GET ") + ServerUri + QStringLiteral("/status/sessions"),
               {.status = 200, .body = PlexFixture(QStringLiteral("plex-sessions.json"))});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    QList<PlexSession> sessions;

    client.listSessions(livingRoom(), [&](PlexApiError, const QList<PlexSession> &found) { sessions = found; });

    for (const auto &session : sessions) QVERIFY(!session.sessionKey.isEmpty());
}

void PlexApiClientTests::listSessionsAcceptsEmptyContainer() {
    FakePlexHttp http;
    http.route(QStringLiteral("GET ") + ServerUri + QStringLiteral("/status/sessions"),
               {.status = 200, .body = R"({"MediaContainer":{"size":0}})"});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    PlexApiError error = PlexApiError::InvalidResponse;
    QList<PlexSession> sessions{PlexSession{}};

    client.listSessions(livingRoom(), [&](const PlexApiError result, const QList<PlexSession> &found) {
        error = result;
        sessions = found;
    });

    QCOMPARE(error, PlexApiError::None);
    QVERIFY(sessions.isEmpty());
}

void PlexApiClientTests::listSessionsReportsUnavailableAndInvalidResponses() {
    const auto key = QStringLiteral("GET ") + ServerUri + QStringLiteral("/status/sessions");
    const QList<QPair<PlexHttpResponse, PlexApiError>> cases{
        {{.status = 0}, PlexApiError::Unavailable},
        {{.status = 503}, PlexApiError::Unavailable},
        {{.status = 401}, PlexApiError::Unauthorized},
        {{.status = 200, .body = "<MediaContainer/>"}, PlexApiError::InvalidResponse},
    };
    for (const auto &[response, expected] : cases) {
        FakePlexHttp http;
        http.route(key, response);
        PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
        PlexApiError error = PlexApiError::None;
        client.listSessions(livingRoom(), [&](const PlexApiError result, const QList<PlexSession> &) { error = result; });
        QCOMPARE(error, expected);
    }

    FakePlexHttp http;
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    PlexApiError error = PlexApiError::None;
    client.listSessions(PlexServer{}, [&](const PlexApiError result, const QList<PlexSession> &) { error = result; });
    QCOMPARE(error, PlexApiError::Unavailable);
    QVERIFY(http.requests.isEmpty());
}

void PlexApiClientTests::listUsersSkipsSystemAccount() {
    FakePlexHttp http;
    http.route(QStringLiteral("GET ") + ServerUri + QStringLiteral("/accounts"),
               {.status = 200, .body = PlexFixture(QStringLiteral("plex-users.json"))});
    PlexApiClient client(http, {.clientIdentifier = QStringLiteral("client-123")});
    QList<PlexAccount> users;

    client.listUsers(livingRoom(), [&](PlexApiError, const QList<PlexAccount> &found) { users = found; });

    QCOMPARE(users.size(), 2);
    QCOMPARE(users.at(0).id, QStringLiteral("1"));
    QCOMPARE(users.at(0).name, QStringLiteral("owner"));
    QCOMPARE(users.at(1).id, QStringLiteral("7"));
}

void PlexApiClientTests::qtHttpTimesOutWithoutResponse() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QList<QTcpSocket *> held;
    connect(&server, &QTcpServer::newConnection, this, [&] { held.append(server.nextPendingConnection()); });
    QNetworkAccessManager network;
    QtPlexHttp http(network, 200);
    std::optional<PlexHttpResponse> response;

    http.send({.url = QUrl(QStringLiteral("http://127.0.0.1:%1/status/sessions").arg(server.serverPort()))},
              [&response](const PlexHttpResponse &value) { response = value; });

    QTRY_VERIFY_WITH_TIMEOUT(response.has_value(), 5000);
    QCOMPARE(response->status, 0);
}

void PlexApiClientTests::qtHttpReturnsStatusAndBody() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QByteArray receivedRequest;
    connect(&server, &QTcpServer::newConnection, this, [&] {
        auto *socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket, &receivedRequest] {
            receivedRequest += socket->readAll();
            if (!receivedRequest.contains("\r\n\r\n")) return;
            socket->write("HTTP/1.1 401 Unauthorized\r\nContent-Type: application/json\r\nContent-Length: 2\r\n"
                          "Connection: close\r\n\r\n{}");
            socket->disconnectFromHost();
        });
    });
    QNetworkAccessManager network;
    QtPlexHttp http(network, 5000);
    std::optional<PlexHttpResponse> response;

    http.send({.url = QUrl(QStringLiteral("http://127.0.0.1:%1/accounts").arg(server.serverPort())),
               .headers = {{"X-Plex-Token", "abc"}}},
              [&response](const PlexHttpResponse &value) { response = value; });

    QTRY_VERIFY_WITH_TIMEOUT(response.has_value(), 5000);
    QCOMPARE(response->status, 401);
    QCOMPARE(response->body, QByteArray("{}"));
    QVERIFY(receivedRequest.toLower().contains("x-plex-token: abc"));
}

QTEST_MAIN(PlexApiClientTests)
#include "PlexApiClientTests.moc"
