#include <QUrlQuery>
#include <QtTest>

#include "../../src/infrastructure/recognition/plex/PlexAuthentication.h"
#include "PlexTestSupport.h"

namespace {
const auto PinsUrl = QStringLiteral("POST https://plex.tv/api/v2/pins");
const auto PinUrl = QStringLiteral("GET https://plex.tv/api/v2/pins/4242");
const auto Token = QStringLiteral("plex-account-token-SECRET");
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

PlexHttpResponse pinCreated(const int expiresIn = 1800) {
    return {.status = 201,
            .body = QStringLiteral(R"({"id":4242,"code":"abcd-code","expiresIn":%1,"authToken":null})")
                        .arg(expiresIn).toUtf8()};
}

PlexHttpResponse pinPending() {
    return {.status = 200, .body = R"({"id":4242,"code":"abcd-code","authToken":null})"};
}

PlexHttpResponse pinClaimed() {
    return {.status = 200,
            .body = QStringLiteral(R"({"id":4242,"code":"abcd-code","authToken":"%1"})").arg(Token).toUtf8()};
}
}

class PlexAuthenticationTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void beginLoginCreatesStrongPinWithClientHeaders();
    void beginLoginOpensOfficialAuthUrl();
    void pollingStoresTokenWhenPinIsClaimed();
    void pollingTimesOutWhenPinExpires();
    void unauthorizedPinCreationFailsSafely();
    void browserFailureCancelsLogin();
    void cancelStopsPolling();
    void disconnectClearsStoredToken();
    void tokenNeverAppearsInMessagesOrLogs();

private:
    PlexAuthentication makeAuthentication();

    FakePlexHttp http;
    FakeSecretStore secrets;
    QList<QUrl> opened;
    bool browserOpens = true;
    QDateTime now;
    int scheduled = 0;
};

void PlexAuthenticationTests::init() {
    http = FakePlexHttp();
    secrets = FakeSecretStore();
    opened.clear();
    browserOpens = true;
    now = Start;
    scheduled = 0;
}

PlexAuthentication PlexAuthenticationTests::makeAuthentication() {
    return PlexAuthentication({.http = &http,
                               .secrets = &secrets,
                               .identity = {.clientIdentifier = QStringLiteral("client-123")},
                               .openUrl = [this](const QUrl &url) {
                                   opened.append(url);
                                   return browserOpens;
                               },
                               .schedule = [this](const int delayMs, std::function<void()> callback) {
                                   ++scheduled;
                                   now = now.addMSecs(delayMs);
                                   callback();
                               },
                               .clock = [this] { return now; }});
}

void PlexAuthenticationTests::beginLoginCreatesStrongPinWithClientHeaders() {
    http.route(PinsUrl, pinCreated());
    http.route(PinUrl, pinClaimed());
    auto authentication = makeAuthentication();

    authentication.beginBrowserLogin([](const PlexLoginResult &) {});

    const auto &request = http.requests.first();
    QCOMPARE(request.method, QByteArray("POST"));
    QCOMPARE(QUrlQuery(request.url).queryItemValue(QStringLiteral("strong")), QStringLiteral("true"));
    QCOMPARE(http.header(request, "X-Plex-Client-Identifier"), QByteArray("client-123"));
    QCOMPARE(http.header(request, "X-Plex-Product"), QByteArray("HaikenAnime"));
    QCOMPARE(http.header(request, "Accept"), QByteArray("application/json"));
}

void PlexAuthenticationTests::beginLoginOpensOfficialAuthUrl() {
    http.route(PinsUrl, pinCreated());
    http.route(PinUrl, pinClaimed());
    auto authentication = makeAuthentication();

    authentication.beginBrowserLogin([](const PlexLoginResult &) {});

    QCOMPARE(opened.size(), 1);
    const auto url = opened.first();
    QCOMPARE(url.host(), QStringLiteral("app.plex.tv"));
    QCOMPARE(url.path(), QStringLiteral("/auth"));
    const auto encoded = url.toString(QUrl::FullyEncoded);
    QVERIFY2(encoded.startsWith(QStringLiteral("https://app.plex.tv/auth#?")), qPrintable(encoded));
    QVERIFY2(encoded.contains(QStringLiteral("clientID=client-123")), qPrintable(encoded));
    QVERIFY2(encoded.contains(QStringLiteral("code=abcd-code")), qPrintable(encoded));
    QVERIFY2(encoded.contains(QStringLiteral("context%5Bdevice%5D%5Bproduct%5D=HaikenAnime")), qPrintable(encoded));
}

void PlexAuthenticationTests::pollingStoresTokenWhenPinIsClaimed() {
    http.route(PinsUrl, pinCreated());
    http.route(PinUrl, pinPending());
    http.route(PinUrl, pinPending());
    http.route(PinUrl, pinClaimed());
    auto authentication = makeAuthentication();
    std::optional<PlexLoginResult> result;

    authentication.beginBrowserLogin([&result](const PlexLoginResult &value) { result = value; });

    QVERIFY(result.has_value());
    QCOMPARE(result->status, PlexLoginStatus::Succeeded);
    QVERIFY(authentication.hasToken());
    QString stored;
    QString error;
    QVERIFY(authentication.readToken(stored, error));
    QCOMPARE(stored, Token);
    QVERIFY(!authentication.isLoginInProgress());
    const auto &poll = http.requests.at(1);
    QCOMPARE(http.header(poll, "X-Plex-Client-Identifier"), QByteArray("client-123"));
}

void PlexAuthenticationTests::pollingTimesOutWhenPinExpires() {
    http.route(PinsUrl, pinCreated(3));
    http.route(PinUrl, pinPending());
    auto authentication = makeAuthentication();
    std::optional<PlexLoginResult> result;

    authentication.beginBrowserLogin([&result](const PlexLoginResult &value) { result = value; });

    QVERIFY(result.has_value());
    QCOMPARE(result->status, PlexLoginStatus::TimedOut);
    QVERIFY(!authentication.hasToken());
    QVERIFY(scheduled <= 4);
}

void PlexAuthenticationTests::unauthorizedPinCreationFailsSafely() {
    http.route(PinsUrl, {.status = 401, .body = R"({"errors":[{"code":1001}]})"});
    auto authentication = makeAuthentication();
    std::optional<PlexLoginResult> result;

    authentication.beginBrowserLogin([&result](const PlexLoginResult &value) { result = value; });

    QCOMPARE(result->status, PlexLoginStatus::Failed);
    QVERIFY(!result->safeMessage.isEmpty());
    QVERIFY(opened.isEmpty());
}

void PlexAuthenticationTests::browserFailureCancelsLogin() {
    browserOpens = false;
    http.route(PinsUrl, pinCreated());
    http.route(PinUrl, pinClaimed());
    auto authentication = makeAuthentication();
    std::optional<PlexLoginResult> result;

    authentication.beginBrowserLogin([&result](const PlexLoginResult &value) { result = value; });

    QCOMPARE(result->status, PlexLoginStatus::Failed);
    QVERIFY(!authentication.hasToken());
}

void PlexAuthenticationTests::cancelStopsPolling() {
    http.route(PinsUrl, pinCreated());
    http.route(PinUrl, pinPending());
    std::function<void()> pending;
    PlexAuthentication authentication({.http = &http,
                                       .secrets = &secrets,
                                       .identity = {.clientIdentifier = QStringLiteral("client-123")},
                                       .openUrl = [](const QUrl &) { return true; },
                                       .schedule = [&pending](int, std::function<void()> callback) {
                                           pending = std::move(callback);
                                       },
                                       .clock = [this] { return now; }});
    std::optional<PlexLoginResult> result;
    authentication.beginBrowserLogin([&result](const PlexLoginResult &value) { result = value; });
    QVERIFY(authentication.isLoginInProgress());

    authentication.cancel();
    const auto requestsBefore = http.requests.size();
    if (pending) pending();

    QCOMPARE(result->status, PlexLoginStatus::Cancelled);
    QCOMPARE(http.requests.size(), requestsBefore);
}

void PlexAuthenticationTests::disconnectClearsStoredToken() {
    secrets.secrets[{QStringLiteral("plex"), QStringLiteral("token")}] = Token;
    auto authentication = makeAuthentication();
    QString error;

    QVERIFY(authentication.disconnect(error));

    QVERIFY(!authentication.hasToken());
}

void PlexAuthenticationTests::tokenNeverAppearsInMessagesOrLogs() {
    LogCapture logs;
    http.route(PinsUrl, pinCreated());
    http.route(PinUrl, pinClaimed());
    auto authentication = makeAuthentication();
    std::optional<PlexLoginResult> result;

    authentication.beginBrowserLogin([&result](const PlexLoginResult &value) { result = value; });

    QVERIFY(!result->safeMessage.contains(Token));
    QVERIFY(!logs.contains(Token));
    for (const auto &url : opened) QVERIFY(!url.toString().contains(Token));
}

QTEST_MAIN(PlexAuthenticationTests)
#include "PlexAuthenticationTests.moc"
