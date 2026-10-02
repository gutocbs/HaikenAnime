#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QTemporaryDir>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

#include "../../src/app/SeasonalCatalogCoordinator.h"
#include "../../src/application/catalog/ISeasonalCatalogDataSource.h"
#include "../../src/infrastructure/anilist/AniListGraphQlPageParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlResponseParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlClient.h"
#include "../../src/infrastructure/anilist/GraphQlQueryStore.h"
#include "../../src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h"

namespace {
Media media(const int id) {
    Media value;
    value.Id = id;
    value.Name = QStringLiteral("Media %1").arg(id);
    return value;
}

MediaPage page(const int currentPage, const int totalPages, const bool hasNextPage,
               std::initializer_list<int> ids) {
    MediaPage value;
    value.currentPage = currentPage;
    value.totalPages = totalPages;
    value.hasNextPage = hasNextPage;
    for (const int id : ids) value.media.append(media(id));
    return value;
}

class RecordingSource final : public ISeasonalCatalogDataSource {
public:
    bool Fetch(const SeasonalCatalogRequest &request, MediaPage &result, QString &error) override {
        requests.append(request);
        if (onFetch) onFetch(request);
        if (errorsByYear.contains(request.year)) {
            error = errorsByYear.value(request.year);
            return false;
        }
        if (!errorToReturn.isEmpty()) {
            error = errorToReturn;
            return false;
        }
        result = pages.value(request.page);
        return true;
    }

    QList<SeasonalCatalogRequest> requests;
    QHash<int, MediaPage> pages;
    QHash<int, QString> errorsByYear;
    QString errorToReturn;
    std::function<void(const SeasonalCatalogRequest &)> onFetch;
};

QByteArray fixture(const QString &name) {
    QFile file(QStringLiteral(HAIKENANIME_SEASONAL_FIXTURE_DIR) + u'/' + name);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return file.readAll();
}

class LocalGraphQlServer final {
public:
    bool start(const QByteArray &responseBody) {
        response_ = QByteArrayLiteral("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ")
            + QByteArray::number(responseBody.size())
            + QByteArrayLiteral("\r\nConnection: close\r\n\r\n") + responseBody;
        QObject::connect(&server_, &QTcpServer::newConnection, &server_, [this] {
            auto *socket = server_.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                request_ += socket->readAll();
                if (!hasResponded_) {
                    hasResponded_ = true;
                    socket->write(response_);
                    socket->disconnectFromHost();
                }
            });
        });
        return server_.listen(QHostAddress::LocalHost);
    }

    [[nodiscard]] QUrl endpoint() const {
        return QUrl(QStringLiteral("http://127.0.0.1:%1/graphql").arg(server_.serverPort()));
    }

    [[nodiscard]] QJsonObject requestPayload() const {
        const auto bodyOffset = request_.indexOf(QByteArrayLiteral("\r\n\r\n"));
        if (bodyOffset < 0) return {};
        return QJsonDocument::fromJson(request_.mid(bodyOffset + 4)).object();
    }

private:
    QTcpServer server_;
    QByteArray request_;
    QByteArray response_;
    bool hasResponded_ = false;
};
}

class SeasonalCatalogTests final : public QObject {
    Q_OBJECT

private slots:
    void requestRejectsInvalidYearSeasonAndPagination();
    void requestAndGraphQlVariablesDistinguishAdultContentPolicy();
    void gateDoesNotCallSourceUntilBothFiltersAreValid();
    void coordinatorPaginatesAndPreservesFirstDuplicateOccurrence();
    void coordinatorStopsAtThePaginationEnvelopeAndHandlesEmptyPage();
    void staleResponseDoesNotReplaceNewerSelection();
    void errorDoesNotReplaceNewerSelectionAndIsReported();
    void coordinatorReusesCachedPageWithoutCallingTheSource();
    void coordinatorCacheKeyDistinguishesAdultPolicyAndPage();
    void coordinatorExpiresAndEvictsCachedPages();
    void coordinatorExpiresCachedPagesFromTheirFetchTime();
    void coordinatorExpiresCachedPagesAtExactTtl();
    void coordinatorCoalescesRapidSelectionsAndSuppressesDuplicateInFlightPage();
    void coordinatorKeepsActiveResultWhenRapidSelectionReturnsToItsIdentity();
    void coordinatorEnforcesDeterministicMinimumRequestInterval();
    void coordinatorRetriesFailuresInsteadOfCachingThem();
    void graphQlDataSourceUsesExternalQueryAndExplicitSeasonVariables();
    void graphQlDataSourcePostsExplicitVariablesAndParsesCatalogResponse();
    void graphQlDataSourcePropagatesGraphQlErrors();
    void graphQlDataSourceRejectsMalformedResponse();
    void seasonalFixturesCoverCompleteEmptyMalformedAndPaginatedPages();
};

void SeasonalCatalogTests::requestRejectsInvalidYearSeasonAndPagination() {
    QVERIFY(!(SeasonalCatalogRequest{0, QStringLiteral("WINTER"), 1, 50}.isValid()));
    QVERIFY(!(SeasonalCatalogRequest{2026, QStringLiteral("AUTUMN"), 1, 50}.isValid()));
    QVERIFY(!(SeasonalCatalogRequest{2026, QStringLiteral("WINTER"), 0, 50}.isValid()));
    QVERIFY(!(SeasonalCatalogRequest{2026, QStringLiteral("WINTER"), 1, 51}.isValid()));
    QVERIFY((SeasonalCatalogRequest{2026, QStringLiteral("WINTER"), 1, 50}.isValid()));
}

void SeasonalCatalogTests::requestAndGraphQlVariablesDistinguishAdultContentPolicy() {
    const SeasonalCatalogRequest disabled{2026, QStringLiteral("WINTER"), 1, 50, false};
    const SeasonalCatalogRequest enabled{2026, QStringLiteral("WINTER"), 1, 50, true};
    const SeasonalCatalogRequest smallerPage{2026, QStringLiteral("WINTER"), 1, 25, false};
    QVERIFY(disabled.isValid());
    QVERIFY(enabled.isValid());
    QVERIFY(!(disabled == enabled));
    QVERIFY(!(disabled == smallerPage));
    QCOMPARE(seasonalCatalogGraphQlVariables(disabled).value(QStringLiteral("includeAdultContent")).toBool(), false);
    QVERIFY(seasonalCatalogGraphQlVariables(enabled).value(QStringLiteral("includeAdultContent")).isNull());
}

void SeasonalCatalogTests::gateDoesNotCallSourceUntilBothFiltersAreValid() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {1}));
    SeasonalCatalogCoordinator coordinator(source);

    coordinator.SetYear(2026);
    QCOMPARE(source.requests.size(), 0);
    coordinator.SetSeason(QStringLiteral("AUTUMN"));
    QCOMPARE(source.requests.size(), 0);
    coordinator.SetSeason(QStringLiteral("SPRING"));
    QCOMPARE(source.requests.size(), 1);
    QCOMPARE(source.requests.first(), (SeasonalCatalogRequest{2026, QStringLiteral("SPRING"), 1, 50}));
    QCOMPARE(coordinator.media().size(), 1);
}

void SeasonalCatalogTests::coordinatorPaginatesAndPreservesFirstDuplicateOccurrence() {
    RecordingSource source;
    source.pages.insert(1, page(1, 2, true, {11, 11, 12}));
    source.pages.insert(2, page(2, 2, false, {12, 13}));
    SeasonalCatalogCoordinator coordinator(source);

    coordinator.SetYear(2026);
    coordinator.SetSeason(QStringLiteral("SUMMER"));
    coordinator.LoadNextPage();

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(source.requests.at(1), (SeasonalCatalogRequest{2026, QStringLiteral("SUMMER"), 2, 50}));
    QCOMPARE(coordinator.media().size(), 3);
    QCOMPARE(coordinator.media().at(0).Id, 11);
    QCOMPARE(coordinator.media().at(1).Id, 12);
    QCOMPARE(coordinator.media().at(2).Id, 13);
}

void SeasonalCatalogTests::coordinatorStopsAtThePaginationEnvelopeAndHandlesEmptyPage() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, true, {}));
    SeasonalCatalogCoordinator coordinator(source);

    coordinator.SetYear(2026);
    coordinator.SetSeason(QStringLiteral("FALL"));
    coordinator.LoadNextPage();

    QCOMPARE(source.requests.size(), 1);
    QVERIFY(coordinator.media().isEmpty());
    QCOMPARE(coordinator.state(), SeasonalCatalogState::Empty);
    QVERIFY(!coordinator.canLoadNextPage());
}

void SeasonalCatalogTests::staleResponseDoesNotReplaceNewerSelection() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {1}));
    SeasonalCatalogCoordinator coordinator(source);
    bool startedNewRequest = false;
    source.onFetch = [&](const SeasonalCatalogRequest &request) {
        if (!startedNewRequest && request.year == 2026) {
            startedNewRequest = true;
            source.pages.insert(1, page(1, 1, false, {2}));
            coordinator.SetYear(2027);
        }
    };

    coordinator.SetSeason(QStringLiteral("WINTER"));
    coordinator.SetYear(2026);

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(coordinator.request().year, 2027);
    QCOMPARE(coordinator.media().size(), 1);
    QCOMPARE(coordinator.media().first().Id, 2);
}

void SeasonalCatalogTests::errorDoesNotReplaceNewerSelectionAndIsReported() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {3}));
    source.errorsByYear.insert(2026, QStringLiteral("old request failed"));
    SeasonalCatalogCoordinator coordinator(source);
    bool startedNewRequest = false;
    source.onFetch = [&](const SeasonalCatalogRequest &request) {
        if (!startedNewRequest && request.year == 2026) {
            startedNewRequest = true;
            source.pages.insert(1, page(1, 1, false, {4}));
            coordinator.SetYear(2027);
        }
    };

    coordinator.SetSeason(QStringLiteral("SPRING"));
    coordinator.SetYear(2026);

    QCOMPARE(coordinator.request().year, 2027);
    QCOMPARE(coordinator.media().first().Id, 4);
    QVERIFY(coordinator.error().isEmpty());

    source.onFetch = {};
    source.errorToReturn = QStringLiteral("network unavailable");
    coordinator.Retry();
    QCOMPARE(coordinator.state(), SeasonalCatalogState::Error);
    QCOMPARE(coordinator.error(), QStringLiteral("network unavailable"));
}

void SeasonalCatalogTests::coordinatorReusesCachedPageWithoutCallingTheSource() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {7}));
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 1000;
    qint64 now = 100;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [&] { return now; });

    coordinator.SetSeason(QStringLiteral("WINTER"));
    coordinator.SetYear(2026);
    coordinator.SetYear(2027);
    coordinator.SetYear(2026);

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(coordinator.request().year, 2026);
    QCOMPARE(coordinator.media().first().Id, 7);
}

void SeasonalCatalogTests::coordinatorCacheKeyDistinguishesAdultPolicyAndPage() {
    RecordingSource source;
    source.pages.insert(1, page(1, 2, true, {1}));
    source.pages.insert(2, page(2, 2, false, {2}));
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 1000;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [] { return qint64{100}; });

    coordinator.SetYear(2026);
    coordinator.SetSeason(QStringLiteral("SPRING"));
    coordinator.LoadNextPage();
    coordinator.SetIncludeAdultContent(true);
    coordinator.SetIncludeAdultContent(false);

    QCOMPARE(source.requests.size(), 3);
    QCOMPARE(source.requests.at(0), (SeasonalCatalogRequest{2026, QStringLiteral("SPRING"), 1, 50, false}));
    QCOMPARE(source.requests.at(1), (SeasonalCatalogRequest{2026, QStringLiteral("SPRING"), 2, 50, false}));
    QCOMPARE(source.requests.at(2), (SeasonalCatalogRequest{2026, QStringLiteral("SPRING"), 1, 50, true}));
}

void SeasonalCatalogTests::coordinatorExpiresAndEvictsCachedPages() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {8}));
    SeasonalCatalogCachePolicy policy;
    policy.maximumEntries = 1;
    policy.timeToLiveMs = 10;
    qint64 now = 0;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [&] { return now; });

    coordinator.SetYear(2026);
    coordinator.SetSeason(QStringLiteral("WINTER"));
    coordinator.SetSeason(QStringLiteral("SPRING"));
    coordinator.SetSeason(QStringLiteral("WINTER"));
    QCOMPARE(source.requests.size(), 3);

    now = 11;
    coordinator.SetSeason(QStringLiteral("SPRING"));
    QCOMPARE(source.requests.size(), 4);
}

void SeasonalCatalogTests::coordinatorExpiresCachedPagesFromTheirFetchTime() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {12}));
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 10;
    policy.minimumRequestIntervalMs = 0;
    qint64 now = 0;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [&] { return now; });

    coordinator.SetSeason(QStringLiteral("FALL"));
    coordinator.SetYear(2026);
    now = 9;
    coordinator.SetYear(2027);
    coordinator.SetYear(2026);
    now = 11;
    coordinator.SetYear(2027);
    coordinator.SetYear(2026);

    QCOMPARE(source.requests.size(), 3);
}

void SeasonalCatalogTests::coordinatorExpiresCachedPagesAtExactTtl() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {12}));
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 10;
    policy.minimumRequestIntervalMs = 0;
    qint64 now = 0;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [&] { return now; });

    coordinator.SetSeason(QStringLiteral("FALL"));
    coordinator.SetYear(2026);
    now = 10;
    coordinator.SetYear(2027);
    coordinator.SetYear(2026);

    QCOMPARE(source.requests.size(), 3);
}

void SeasonalCatalogTests::coordinatorCoalescesRapidSelectionsAndSuppressesDuplicateInFlightPage() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {9}));
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 1000;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [] { return qint64{100}; });
    bool changedSelection = false;
    source.onFetch = [&](const SeasonalCatalogRequest &) {
        if (changedSelection) return;
        changedSelection = true;
        coordinator.Retry();
        coordinator.SetYear(2027);
        coordinator.SetYear(2028);
    };

    coordinator.SetSeason(QStringLiteral("SUMMER"));
    coordinator.SetYear(2026);

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(source.requests.first().year, 2026);
    QCOMPARE(source.requests.last().year, 2028);
    QCOMPARE(coordinator.request().year, 2028);
    QCOMPARE(coordinator.media().first().Id, 9);
}

void SeasonalCatalogTests::coordinatorKeepsActiveResultWhenRapidSelectionReturnsToItsIdentity() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {11}));
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 1000;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [] { return qint64{100}; });
    bool changedSelection = false;
    source.onFetch = [&](const SeasonalCatalogRequest &) {
        if (changedSelection) return;
        changedSelection = true;
        coordinator.SetYear(2027);
        coordinator.SetYear(2026);
    };

    coordinator.SetSeason(QStringLiteral("WINTER"));
    coordinator.SetYear(2026);

    QCOMPARE(source.requests.size(), 1);
    QCOMPARE(coordinator.request().year, 2026);
    QCOMPARE(coordinator.state(), SeasonalCatalogState::Populated);
    QCOMPARE(coordinator.media().first().Id, 11);
}

void SeasonalCatalogTests::coordinatorEnforcesDeterministicMinimumRequestInterval() {
    RecordingSource source;
    source.pages.insert(1, page(1, 1, false, {10}));
    SeasonalCatalogCachePolicy policy;
    policy.minimumRequestIntervalMs = 100;
    qint64 now = 0;
    QList<qint64> delays;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [&] { return now; },
                                           [&](const qint64 delay) { delays.append(delay); now += delay; });

    coordinator.SetSeason(QStringLiteral("FALL"));
    coordinator.SetYear(2026);
    coordinator.SetYear(2027);

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(delays, (QList<qint64>{100}));
}

void SeasonalCatalogTests::coordinatorRetriesFailuresInsteadOfCachingThem() {
    RecordingSource source;
    source.errorToReturn = QStringLiteral("unavailable");
    SeasonalCatalogCachePolicy policy;
    policy.timeToLiveMs = 1000;
    SeasonalCatalogCoordinator coordinator(source, 50, nullptr, policy, [] { return qint64{100}; });

    coordinator.SetYear(2026);
    coordinator.SetSeason(QStringLiteral("FALL"));
    coordinator.Retry();

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(coordinator.state(), SeasonalCatalogState::Error);
}

void SeasonalCatalogTests::graphQlDataSourceUsesExternalQueryAndExplicitSeasonVariables() {
    GraphQlQueryStore store(QStringLiteral(":/anilist/queries/seasonal-catalog.graphql"));
    QString error;
    QString query;
    QVERIFY2(store.load(query, error), qPrintable(error));
    const auto variables = seasonalCatalogGraphQlVariables({2026, QStringLiteral("FALL"), 1, 25});
    QCOMPARE(variables.value(QStringLiteral("year")).toInt(), 2026);
    QCOMPARE(variables.value(QStringLiteral("season")).toString(), QStringLiteral("FALL"));
    QCOMPARE(variables.value(QStringLiteral("page")).toInt(), 1);
    QCOMPARE(variables.value(QStringLiteral("perPage")).toInt(), 25);
    QVERIFY(query.contains(QStringLiteral("SeasonalCatalog")));
    QVERIFY(query.contains(QStringLiteral("seasonYear: $year")));
}

void SeasonalCatalogTests::graphQlDataSourcePostsExplicitVariablesAndParsesCatalogResponse() {
    LocalGraphQlServer server;
    QVERIFY2(server.start(fixture(QStringLiteral("seasonal-complete.json"))), "Local GraphQL server did not start.");
    QNetworkAccessManager networkManager;
    AniListGraphQlClient client(networkManager, nullptr, server.endpoint(), 1000);
    GraphQlQueryStore store(QStringLiteral(":/anilist/queries/seasonal-catalog.graphql"));
    GraphQlSeasonalCatalogDataSource source(client, store);
    MediaPage result;
    QString error;

    QVERIFY2(source.Fetch({2026, QStringLiteral("FALL"), 2, 25}, result, error), qPrintable(error));
    QCOMPARE(result.currentPage, 1);
    QCOMPARE(result.media.size(), 1);
    QCOMPARE(result.media.first().Id, 44);

    const auto payload = server.requestPayload();
    QVERIFY(payload.value(QStringLiteral("query")).toString().contains(QStringLiteral("SeasonalCatalog")));
    const auto variables = payload.value(QStringLiteral("variables")).toObject();
    QCOMPARE(variables.value(QStringLiteral("year")).toInt(), 2026);
    QCOMPARE(variables.value(QStringLiteral("season")).toString(), QStringLiteral("FALL"));
    QCOMPARE(variables.value(QStringLiteral("page")).toInt(), 2);
    QCOMPARE(variables.value(QStringLiteral("perPage")).toInt(), 25);
}

void SeasonalCatalogTests::graphQlDataSourcePropagatesGraphQlErrors() {
    LocalGraphQlServer server;
    QVERIFY2(server.start(fixture(QStringLiteral("error-response.json"))), "Local GraphQL server did not start.");
    QNetworkAccessManager networkManager;
    AniListGraphQlClient client(networkManager, nullptr, server.endpoint(), 1000);
    GraphQlQueryStore store(QStringLiteral(":/anilist/queries/seasonal-catalog.graphql"));
    GraphQlSeasonalCatalogDataSource source(client, store);
    MediaPage result;
    QString error;

    QVERIFY(!source.Fetch({2026, QStringLiteral("WINTER"), 1, 50}, result, error));
    QCOMPARE(error, QStringLiteral("Invalid credentials"));
    QVERIFY(result.media.isEmpty());
}

void SeasonalCatalogTests::graphQlDataSourceRejectsMalformedResponse() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto queryPath = directory.filePath(QStringLiteral("seasonal-catalog.graphql"));
    QFile queryFile(queryPath);
    QVERIFY(queryFile.open(QIODevice::WriteOnly | QIODevice::Text));
    queryFile.write("query SeasonalCatalog { Page { pageInfo { currentPage } } }");
    queryFile.close();
    GraphQlQueryStore store(queryPath);
    QString error;
    QString query;
    QVERIFY2(store.load(query, error), qPrintable(error));
    AniListGraphQlResponse response;
    QVERIFY2(AniListGraphQlResponseParser::parse(fixture(QStringLiteral("seasonal-malformed.json")), response, error), qPrintable(error));
    MediaPage result;
    QVERIFY(!AniListGraphQlPageParser::parse(response.data, result, error));
    QVERIFY(!error.isEmpty());
}

void SeasonalCatalogTests::seasonalFixturesCoverCompleteEmptyMalformedAndPaginatedPages() {
    const auto completePayload = fixture(QStringLiteral("seasonal-complete.json"));
    const auto emptyPayload = fixture(QStringLiteral("seasonal-empty.json"));
    const auto malformedPayload = fixture(QStringLiteral("seasonal-malformed.json"));
    const auto secondPagePayload = fixture(QStringLiteral("seasonal-page-two.json"));
    QVERIFY(!completePayload.isEmpty());
    QVERIFY(!emptyPayload.isEmpty());
    QVERIFY(!malformedPayload.isEmpty());
    QVERIFY(!secondPagePayload.isEmpty());

    AniListGraphQlResponse response;
    MediaPage parsed;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(completePayload, response, error), qPrintable(error));
    QVERIFY2(AniListGraphQlPageParser::parse(response.data, parsed, error), qPrintable(error));
    QCOMPARE(parsed.media.size(), 1);
    QCOMPARE(parsed.media.first().Id, 44);

    QVERIFY2(AniListGraphQlResponseParser::parse(emptyPayload, response, error), qPrintable(error));
    QVERIFY2(AniListGraphQlPageParser::parse(response.data, parsed, error), qPrintable(error));
    QVERIFY(parsed.media.isEmpty());

    QVERIFY2(AniListGraphQlResponseParser::parse(secondPagePayload, response, error), qPrintable(error));
    QVERIFY2(AniListGraphQlPageParser::parse(response.data, parsed, error), qPrintable(error));
    QCOMPARE(parsed.currentPage, 2);
    QCOMPARE(parsed.media.size(), 2);

    QVERIFY2(AniListGraphQlResponseParser::parse(malformedPayload, response, error), qPrintable(error));
    QVERIFY(!AniListGraphQlPageParser::parse(response.data, parsed, error));
    QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(SeasonalCatalogTests)
#include "SeasonalCatalogTests.moc"
