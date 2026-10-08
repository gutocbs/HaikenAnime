#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryFile>

#include <limits>

#include "../../src/infrastructure/anilist/AniListGraphQlPageParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlResponseParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlClient.h"
#include "../../src/infrastructure/anilist/AniListMediaMapper.h"
#include "../../src/infrastructure/anilist/GraphQlAniListDataSource.h"
#include "../../src/infrastructure/anilist/GraphQlQueryStore.h"
#include "../../src/infrastructure/anilist/RecordedGraphQlAniListDataSource.h"

namespace {
QByteArray fixture(const QString &path) {
    QFile file(path);
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
                if (!responded_) {
                    responded_ = true;
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
    bool responded_ = false;
};
}

class AniListGraphQlParsingTests final : public QObject {
    Q_OBJECT

private slots:
    void parsesDataEnvelopeAndMediaPage();
    void preservesGraphQlErrorsWhenDataIsNull();
    void rejectsEnvelopeWithoutDataOrErrors();
    void clearsPartialDataWhenEnvelopeIsMalformed();
    void rejectsMalformedPageInsteadOfReturningAnEmptySuccess();
    void preservesEveryCoverVariant();
    void fallsBackToAvailableLargeCoverWhenMediumIsMissing();
    void mapsExtendedMetadataAndNormalizesPresentationData();
    void omitsInvalidAndMissingExtendedMetadata();
    void omitsMalformedNumericExtendedMetadata();
    void canonicalizesAndDeduplicatesExternalLinks();
    void recordedLibraryFixtureIsComplete();
    void recordedFixtureCachesOnlyTheAffectedPartitionAndQueryIdentity();
    void recordedFixtureCacheSeparatesUsersAfterFilterMutation();
    void recordedUserListMarksAnUnfilteredFirstPageAuthoritativeBeforeTheTerminalPage();
    void recordedUserListRejectsMissingPaginationMetadata();
    void recordedSourceSelectsTheResponseShapeForTheRequestedPartition();
    void recordedCatalogSourceRetainsOnlyTheRequestedPartitionStatus();
    void graphQlAdapterPostsPartitionVariablesAndParsesCatalogPage();
    void graphQlAdapterRequiresAndPostsAUserNameForUserListRefresh();
    void graphQlAdapterRejectsNonBooleanUserListPaginationMetadata();
};

void AniListGraphQlParsingTests::parsesDataEnvelopeAndMediaPage() {
    const QByteArray payload = R"json({
        "data": {
            "Page": {
                "pageInfo": {"currentPage": 1, "lastPage": 2, "hasNextPage": true},
                "media": [{
                    "id": 154587,
                    "type": "MANGA",
                    "format": "NOVEL",
                    "status": "FINISHED",
                    "userListStatus": "CURRENT",
                    "title": {
                        "romaji": "Sousou no Frieren",
                        "english": "Frieren: Beyond Journey's End",
                        "native": "葬送のフリーレン"
                    },
                    "synonyms": ["Frieren at the Funeral"],
                    "episodes": null,
                    "chapters": 64,
                    "progress": 18,
                    "score": 9,
                    "averageScore": 91,
                    "coverImage": {"large": "https://example.invalid/frieren.png"},
                    "description": "A mage elf continues after the hero's journey.",
                    "siteUrl": "https://anilist.co/manga/154587"
                }]
            }
        }
    })json";

    AniListGraphQlResponse response;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(payload, response, error), qPrintable(error));
    QVERIFY(response.hasData());
    QVERIFY(!response.hasErrors());

    MediaPage page;
    QVERIFY2(AniListGraphQlPageParser::parse(response.data, page, error), qPrintable(error));
    QCOMPARE(page.currentPage, 1);
    QCOMPARE(page.totalPages, 2);
    QVERIFY(page.hasNextPage);
    QCOMPARE(page.media.size(), 1);
    QCOMPARE(page.media.first().Id, 154587);
    QCOMPARE(page.media.first().Name, QStringLiteral("Sousou no Frieren"));
    QCOMPARE(page.media.first().AlternativeNames,
             QStringList{QStringLiteral("Frieren at the Funeral")});
    QCOMPARE(page.media.first().Type, MediaType::Novel);
    QCOMPARE(page.media.first().TotalChapters, 64);
    QCOMPARE(page.media.first().ConsumedChapters, 18);
    QCOMPARE(page.media.first().PersonalScore, 9);
    QCOMPARE(page.media.first().ListStatus, UserListStatus::Current);
}

void AniListGraphQlParsingTests::preservesGraphQlErrorsWhenDataIsNull() {
    const QByteArray payload = R"json({
        "data": null,
        "errors": [{"message": "Temporarily unavailable", "path": ["Page"]}]
    })json";

    AniListGraphQlResponse response;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(payload, response, error), qPrintable(error));
    QVERIFY(!response.hasData());
    QVERIFY(response.hasErrors());
    QCOMPARE(response.errors.first().message, QStringLiteral("Temporarily unavailable"));
    QCOMPARE(response.errors.first().path, QStringList{QStringLiteral("Page")});
}

void AniListGraphQlParsingTests::rejectsEnvelopeWithoutDataOrErrors() {
    AniListGraphQlResponse response;
    QString error;

    QVERIFY(!AniListGraphQlResponseParser::parse(QByteArrayLiteral(R"json({"data": null})json"),
                                                  response, error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!response.hasData());
    QVERIFY(!response.hasErrors());
}

void AniListGraphQlParsingTests::clearsPartialDataWhenEnvelopeIsMalformed() {
    AniListGraphQlResponse response;
    response.dataWasPresent = true;
    response.data.insert(QStringLiteral("stale"), true);
    QString error;

    QVERIFY(!AniListGraphQlResponseParser::parse(
        QByteArrayLiteral(R"json({"data":{"Page":{}},"errors":{}})json"), response, error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!response.hasData());
    QVERIFY(response.data.isEmpty());
    QVERIFY(!response.hasErrors());
}

void AniListGraphQlParsingTests::rejectsMalformedPageInsteadOfReturningAnEmptySuccess() {
    const QJsonObject data{{QStringLiteral("Page"),
                            QJsonObject{{QStringLiteral("pageInfo"),
                                         QJsonObject{{QStringLiteral("currentPage"), 1},
                                                     {QStringLiteral("lastPage"), 1}}},
                                        {QStringLiteral("media"), QJsonArray{}}}}};
    MediaPage page;
    QString error;

    QVERIFY(!AniListGraphQlPageParser::parse(data, page, error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(page.currentPage, 0);
    QVERIFY(page.media.isEmpty());
}

void AniListGraphQlParsingTests::preservesEveryCoverVariant() {
    const QJsonObject object{
        {QStringLiteral("coverImage"),
         QJsonObject{{QStringLiteral("medium"), QStringLiteral("https://img/medium.jpg")},
                     {QStringLiteral("large"), QStringLiteral("https://img/large.jpg")},
                     {QStringLiteral("extraLarge"), QJsonValue::Null}}}};

    const auto media = AniListMediaMapper::FromGraphQlJson(object);

    QCOMPARE(media.coverImages.medium, QStringLiteral("https://img/medium.jpg"));
    QCOMPARE(media.coverImages.large, QStringLiteral("https://img/large.jpg"));
    QVERIFY(media.coverImages.extraLarge.isEmpty());
}

void AniListGraphQlParsingTests::fallsBackToAvailableLargeCoverWhenMediumIsMissing() {
    AniListCoverImagesDto images;
    images.large = QStringLiteral("https://img/large.jpg");

    QCOMPARE(AniListMediaMapper::SelectCoverUrl(images, CoverQuality::Medium),
             QStringLiteral("https://img/large.jpg"));
}

void AniListGraphQlParsingTests::mapsExtendedMetadataAndNormalizesPresentationData() {
    const QJsonObject object{
        {QStringLiteral("season"), QStringLiteral("FALL")},
        {QStringLiteral("seasonYear"), 2026},
        {QStringLiteral("description"),
         QStringLiteral("<p>First <b>paragraph</b>.</p><p>Second &amp; final.<br>Line</p>")},
        {QStringLiteral("siteUrl"), QStringLiteral("https://anilist.co/anime/42")},
        {QStringLiteral("nextAiringEpisode"),
         QJsonObject{{QStringLiteral("episode"), 7},
                     {QStringLiteral("airingAt"), 1790518560}}},
        {QStringLiteral("externalLinks"),
         QJsonArray{
             QJsonObject{{QStringLiteral("site"), QStringLiteral(" Official ")},
                         {QStringLiteral("url"), QStringLiteral("https://example.test/info")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("official")},
                         {QStringLiteral("url"), QStringLiteral("https://example.test/info")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("Unsupported")},
                         {QStringLiteral("url"), QStringLiteral("ftp://example.test/file")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("Relative")},
                         {QStringLiteral("url"), QStringLiteral("/watch")}}}},
        {QStringLiteral("streamingEpisodes"),
         QJsonArray{
             QJsonObject{{QStringLiteral("site"), QStringLiteral("Stream")},
                         {QStringLiteral("url"), QStringLiteral("http://stream.example/watch")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("stream")},
                         {QStringLiteral("url"), QStringLiteral("http://stream.example/watch")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("Javascript")},
                         {QStringLiteral("url"), QStringLiteral("javascript:alert(1)")}}}}
    };

    const auto media = AniListMediaMapper::ToDomainMedia(
        AniListMediaMapper::FromGraphQlJson(object));

    QCOMPARE(media.Season, QStringLiteral("FALL"));
    QVERIFY(media.SeasonYear.has_value());
    QCOMPARE(media.SeasonYear.value(), 2026);
    QVERIFY(media.NextAiringEpisode.has_value());
    QCOMPARE(media.NextAiringEpisode.value(), 7);
    QVERIFY(media.NextAiringAt.has_value());
    QCOMPARE(media.NextAiringAt.value(), qint64(1790518560));
    QCOMPARE(media.AniListUrl, QStringLiteral("https://anilist.co/anime/42"));
    QCOMPARE(media.Synopsis, QStringLiteral("First paragraph.\n\nSecond & final.\nLine"));
    QCOMPARE(media.ExternalLinks.size(), 2);
    QCOMPARE(media.ExternalLinks.at(0).Site, QStringLiteral("Official"));
    QCOMPARE(media.ExternalLinks.at(0).Url, QStringLiteral("https://example.test/info"));
    QCOMPARE(media.ExternalLinks.at(1).Site, QStringLiteral("Stream"));
    QCOMPARE(media.ExternalLinks.at(1).Url, QStringLiteral("http://stream.example/watch"));
}

void AniListGraphQlParsingTests::omitsInvalidAndMissingExtendedMetadata() {
    const QJsonObject object{
        {QStringLiteral("description"), QJsonValue::Null},
        {QStringLiteral("siteUrl"), QStringLiteral("file:///tmp/media")},
        {QStringLiteral("nextAiringEpisode"), QJsonValue::Null},
        {QStringLiteral("externalLinks"),
         QJsonArray{QJsonObject{{QStringLiteral("site"), QStringLiteral("Missing URL")},
                                {QStringLiteral("url"), QJsonValue::Null}}}}
    };

    const auto media = AniListMediaMapper::ToDomainMedia(
        AniListMediaMapper::FromGraphQlJson(object));

    QVERIFY(media.Season.isEmpty());
    QVERIFY(!media.SeasonYear.has_value());
    QVERIFY(!media.NextAiringEpisode.has_value());
    QVERIFY(!media.NextAiringAt.has_value());
    QVERIFY(media.AniListUrl.isEmpty());
    QVERIFY(media.ExternalLinks.isEmpty());
    QVERIFY(media.Synopsis.isEmpty());
}

void AniListGraphQlParsingTests::omitsMalformedNumericExtendedMetadata() {
    const QJsonObject object{
        {QStringLiteral("seasonYear"), QStringLiteral("2026")},
        {QStringLiteral("nextAiringEpisode"),
         QJsonObject{{QStringLiteral("episode"), QStringLiteral("7")},
                     {QStringLiteral("airingAt"), QStringLiteral("1790518560")}}}
    };

    const auto wrongTypedMedia = AniListMediaMapper::ToDomainMedia(
        AniListMediaMapper::FromGraphQlJson(object));

    QVERIFY(!wrongTypedMedia.SeasonYear.has_value());
    QVERIFY(!wrongTypedMedia.NextAiringEpisode.has_value());
    QVERIFY(!wrongTypedMedia.NextAiringAt.has_value());

    const QJsonObject invalidNumbers{
        {QStringLiteral("seasonYear"), std::numeric_limits<double>::max()},
        {QStringLiteral("nextAiringEpisode"),
         QJsonObject{{QStringLiteral("episode"), 7.5},
                     {QStringLiteral("airingAt"), std::numeric_limits<double>::max()}}}
    };

    const auto invalidNumberMedia = AniListMediaMapper::ToDomainMedia(
        AniListMediaMapper::FromGraphQlJson(invalidNumbers));

    QVERIFY(!invalidNumberMedia.SeasonYear.has_value());
    QVERIFY(!invalidNumberMedia.NextAiringEpisode.has_value());
    QVERIFY(!invalidNumberMedia.NextAiringAt.has_value());
}

void AniListGraphQlParsingTests::canonicalizesAndDeduplicatesExternalLinks() {
    const QJsonObject object{
        {QStringLiteral("externalLinks"),
         QJsonArray{
             QJsonObject{{QStringLiteral("site"), QStringLiteral(" Official ")},
                         {QStringLiteral("url"),
                          QStringLiteral("HTTPS://EXAMPLE.TEST:443/info#overview")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("official")},
                         {QStringLiteral("url"), QStringLiteral("https://example.test/info")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("Stream")},
                         {QStringLiteral("url"),
                          QStringLiteral("HTTP://STREAM.EXAMPLE:80/watch#episode")}},
             QJsonObject{{QStringLiteral("site"), QStringLiteral("stream")},
                         {QStringLiteral("url"), QStringLiteral("http://stream.example/watch")}}}}
    };

    const auto media = AniListMediaMapper::ToDomainMedia(
        AniListMediaMapper::FromGraphQlJson(object));

    QCOMPARE(media.ExternalLinks.size(), 2);
    QCOMPARE(media.ExternalLinks.at(0).Site, QStringLiteral("Official"));
    QCOMPARE(media.ExternalLinks.at(0).Url, QStringLiteral("https://example.test/info"));
    QCOMPARE(media.ExternalLinks.at(1).Site, QStringLiteral("Stream"));
    QCOMPARE(media.ExternalLinks.at(1).Url, QStringLiteral("http://stream.example/watch"));
}

void AniListGraphQlParsingTests::recordedLibraryFixtureIsComplete() {
    QFile file(QStringLiteral(HAIKENANIME_GRAPHQL_FIXTURE));
    QVERIFY(file.open(QIODevice::ReadOnly));
    AniListGraphQlResponse response;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(file.readAll(), response, error), qPrintable(error));
    MediaPage page;
    QVERIFY2(AniListGraphQlPageParser::parse(response.data, page, error), qPrintable(error));
    QCOMPARE(page.currentPage, 1);
    QVERIFY(!page.hasNextPage);
    QCOMPARE(page.media.size(), 50);

    int animeCount = 0;
    int mangaCount = 0;
    int novelCount = 0;
    int mangaOrNovelWithoutListStatus = 0;
    int entriesWithProgress = 0;
    int entriesWithPersonalScore = 0;
    int entriesWithSeason = 0;
    int entriesWithNextAiring = 0;
    int entriesWithExternalLinks = 0;
    for (const auto &media : page.media) {
        if (media.Type == MediaType::Anime) ++animeCount;
        if (media.Type == MediaType::Manga) ++mangaCount;
        if (media.Type == MediaType::Novel) ++novelCount;
        if ((media.Type == MediaType::Manga || media.Type == MediaType::Novel)
            && media.ListStatus == UserListStatus::Unknown) {
            ++mangaOrNovelWithoutListStatus;
        }
        if (media.ConsumedChapters > 0) ++entriesWithProgress;
        if (media.PersonalScore > 0) ++entriesWithPersonalScore;
        if (!media.Season.isEmpty() && media.SeasonYear.has_value()) ++entriesWithSeason;
        if (media.NextAiringEpisode.has_value() && media.NextAiringAt.has_value()) {
            ++entriesWithNextAiring;
        }
        if (!media.ExternalLinks.isEmpty()) ++entriesWithExternalLinks;
        QVERIFY(!media.Synopsis.contains(QStringLiteral("<br"), Qt::CaseInsensitive));
    }

    QVERIFY(animeCount > 0);
    QVERIFY(mangaCount >= 3);
    QVERIFY(novelCount >= 3);
    QCOMPARE(mangaOrNovelWithoutListStatus, 0);
    QVERIFY(entriesWithProgress >= 8);
    QVERIFY(entriesWithPersonalScore >= 8);
    QVERIFY(entriesWithSeason > 0);
    QVERIFY(entriesWithNextAiring > 0);
    QVERIFY(entriesWithExternalLinks > 0);
}

void AniListGraphQlParsingTests::recordedFixtureCachesOnlyTheAffectedPartitionAndQueryIdentity() {
    RecordedGraphQlAniListDataSource source(QStringLiteral(HAIKENANIME_GRAPHQL_FIXTURE));
    QString error;
    AniListDataSourceResult result;
    auto active = AniListDataSourceRequest::ForPartition(SyncPartition::ActiveCatalog);
    const auto inactive = AniListDataSourceRequest::ForPartition(SyncPartition::InactiveCatalog);

    QVERIFY2(source.fetchPage(active, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::ActiveCatalog);
    QVERIFY(!result.isCompleteAuthoritativeSnapshot);
    QCOMPARE(source.fixtureReadCount(), 1);
    QCOMPARE(source.externalCallCount(), 0);

    QVERIFY2(source.fetchPage(inactive, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::InactiveCatalog);
    QCOMPARE(source.fixtureReadCount(), 2);
    QVERIFY2(source.fetchPage(inactive, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 2);

    active.queryIdentity = QStringLiteral("catalog:v2");
    QVERIFY2(source.fetchPage(active, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 3);
    QVERIFY2(source.fetchPage(inactive, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 3);

    active.refresh = true;
    QVERIFY2(source.fetchPage(active, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 4);
    QCOMPARE(source.externalCallCount(), 0);

    RecordedGraphQlAniListDataSource userListSource(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE));
    auto userList = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    userList.filter.username = QStringLiteral("fixture-user");
    QVERIFY2(userListSource.fetchPage(userList, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::UserList);
    QVERIFY(result.isCompleteAuthoritativeSnapshot);
}

void AniListGraphQlParsingTests::recordedFixtureCacheSeparatesUsersAfterFilterMutation() {
    RecordedGraphQlAniListDataSource source(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE));
    auto firstUser = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    firstUser.filter.username = QStringLiteral("first-user");
    auto secondUser = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    secondUser.filter.username = QStringLiteral("second-user");
    AniListDataSourceResult result;
    QString error;

    QVERIFY2(source.fetchPage(firstUser, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 1);
    QVERIFY2(source.fetchPage(secondUser, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 2);
    QVERIFY2(source.fetchPage(firstUser, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 2);
    QVERIFY2(source.fetchPage(secondUser, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 2);

    secondUser.filter.list = QStringLiteral("CURRENT");
    QVERIFY2(source.fetchPage(secondUser, result, error), qPrintable(error));
    QCOMPARE(source.fixtureReadCount(), 3);
    QVERIFY(!result.isCompleteAuthoritativeSnapshot);
}

void AniListGraphQlParsingTests::recordedUserListMarksAnUnfilteredFirstPageAuthoritativeBeforeTheTerminalPage() {
    QTemporaryFile multiPageFixture;
    QVERIFY(multiPageFixture.open());
    auto payload = fixture(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE));
    QVERIFY(payload.contains("\"hasNextChunk\": false"));
    payload.replace("\"hasNextChunk\": false", "\"hasNextChunk\": true");
    QVERIFY(multiPageFixture.write(payload) == payload.size());
    multiPageFixture.flush();
    RecordedGraphQlAniListDataSource source(multiPageFixture.fileName());
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.username = QStringLiteral("fixture-user");
    AniListDataSourceResult result;
    QString error;

    QVERIFY2(source.fetchPage(request, result, error), qPrintable(error));
    QVERIFY(result.page.hasNextPage);
    QVERIFY(result.isCompleteAuthoritativeSnapshot);
}

void AniListGraphQlParsingTests::recordedUserListRejectsMissingPaginationMetadata() {
    QTemporaryFile malformedFixture;
    QVERIFY(malformedFixture.open());
    auto payload = fixture(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE));
    QVERIFY(payload.contains("\"hasNextChunk\": false,"));
    payload.replace("\"hasNextChunk\": false,", "");
    QVERIFY(malformedFixture.write(payload) == payload.size());
    malformedFixture.flush();
    RecordedGraphQlAniListDataSource source(malformedFixture.fileName());
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.username = QStringLiteral("fixture-user");
    AniListDataSourceResult result;
    QString error;

    QVERIFY(!source.fetchPage(request, result, error));
    QVERIFY(error.contains(QStringLiteral("hasNextChunk")));
    QVERIFY(!result.isCompleteAuthoritativeSnapshot);
}

void AniListGraphQlParsingTests::recordedSourceSelectsTheResponseShapeForTheRequestedPartition() {
    QString error;
    AniListDataSourceResult result;
    RecordedGraphQlAniListDataSource catalogSource(QStringLiteral(HAIKENANIME_GRAPHQL_FIXTURE));
    const auto catalog = AniListDataSourceRequest::ForPartition(SyncPartition::CompletedCatalog);

    QVERIFY2(catalogSource.fetchPage(catalog, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::CompletedCatalog);
    QVERIFY(!result.page.media.isEmpty());

    RecordedGraphQlAniListDataSource userListSource(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE));
    auto userList = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    userList.filter.username = QStringLiteral("fixture-user");
    QVERIFY2(userListSource.fetchPage(userList, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::UserList);
    QVERIFY(result.isCompleteAuthoritativeSnapshot);

    QCOMPARE(result.page.media.first().Id, 21366);
}

void AniListGraphQlParsingTests::recordedCatalogSourceRetainsOnlyTheRequestedPartitionStatus() {
    RecordedGraphQlAniListDataSource source(QStringLiteral(HAIKENANIME_GRAPHQL_FIXTURE));
    QString error;
    AniListDataSourceResult activeResult;
    AniListDataSourceResult inactiveResult;
    AniListDataSourceResult completedResult;

    QVERIFY2(source.fetchPage(AniListDataSourceRequest::ForPartition(SyncPartition::ActiveCatalog),
                              activeResult, error), qPrintable(error));
    QVERIFY2(source.fetchPage(AniListDataSourceRequest::ForPartition(SyncPartition::InactiveCatalog),
                              inactiveResult, error), qPrintable(error));
    QVERIFY2(source.fetchPage(AniListDataSourceRequest::ForPartition(SyncPartition::CompletedCatalog),
                              completedResult, error), qPrintable(error));

    QCOMPARE(activeResult.completedPartition, SyncPartition::ActiveCatalog);
    QCOMPARE(inactiveResult.completedPartition, SyncPartition::InactiveCatalog);
    QCOMPARE(completedResult.completedPartition, SyncPartition::CompletedCatalog);
    QVERIFY(!activeResult.page.media.isEmpty());
    QVERIFY(inactiveResult.page.media.isEmpty());
    QVERIFY(!completedResult.page.media.isEmpty());
    for (const auto &media : activeResult.page.media) {
        QCOMPARE(media.Status, MediaStatus::Releasing);
    }
    for (const auto &media : completedResult.page.media) {
        QCOMPARE(media.Status, MediaStatus::Released);
    }
}

void AniListGraphQlParsingTests::graphQlAdapterPostsPartitionVariablesAndParsesCatalogPage() {
    LocalGraphQlServer server;
    QVERIFY2(server.start(fixture(QStringLiteral(HAIKENANIME_GRAPHQL_FIXTURE))), "Local GraphQL server did not start.");
    QNetworkAccessManager networkManager;
    AniListGraphQlClient client(networkManager, nullptr, server.endpoint(), 1000);
    GraphQlQueryStore store(QStringLiteral(":/anilist/queries/media-page.graphql"));
    GraphQlAniListDataSource source(client, store);
    const auto request = AniListDataSourceRequest::ForPartition(SyncPartition::ActiveCatalog);
    AniListDataSourceResult result;
    QString error;

    QVERIFY2(source.fetchPage(request, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::ActiveCatalog);
    QVERIFY(!result.isCompleteAuthoritativeSnapshot);
    QCOMPARE(result.page.currentPage, 1);
    QVERIFY(!result.page.media.isEmpty());

    const auto variables = server.requestPayload().value(QStringLiteral("variables")).toObject();
    QCOMPARE(variables.value(QStringLiteral("type")).toString(), QStringLiteral("ANIME"));
    QCOMPARE(variables.value(QStringLiteral("status")).toString(), QStringLiteral("RELEASING"));
    QVERIFY(variables.value(QStringLiteral("includeCatalog")).toBool());
    QVERIFY(!variables.value(QStringLiteral("includeUserList")).toBool());
}

void AniListGraphQlParsingTests::graphQlAdapterRequiresAndPostsAUserNameForUserListRefresh() {
    QNetworkAccessManager networkManager;
    AniListGraphQlClient client(networkManager, nullptr, QUrl(QStringLiteral("http://127.0.0.1:9/graphql")), 1000);
    GraphQlQueryStore store(QStringLiteral(":/anilist/queries/media-page.graphql"));
    GraphQlAniListDataSource source(client, store);
    const auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    AniListDataSourceResult result;
    QString error;

    QVERIFY(!source.fetchPage(request, result, error));
    QVERIFY(error.contains(QStringLiteral("user name"), Qt::CaseInsensitive));

    LocalGraphQlServer server;
    QVERIFY2(server.start(fixture(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE))), "Local GraphQL server did not start.");
    QNetworkAccessManager userListNetworkManager;
    AniListGraphQlClient userListClient(userListNetworkManager, nullptr, server.endpoint(), 1000);
    GraphQlAniListDataSource userListSource(userListClient, store);
    auto namedRequest = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    namedRequest.filter.username = QStringLiteral("fixture-user");

    QVERIFY2(userListSource.fetchPage(namedRequest, result, error), qPrintable(error));
    QCOMPARE(result.completedPartition, SyncPartition::UserList);
    QVERIFY(result.isCompleteAuthoritativeSnapshot);
    const auto variables = server.requestPayload().value(QStringLiteral("variables")).toObject();
    QCOMPARE(variables.value(QStringLiteral("userName")).toString(), QStringLiteral("fixture-user"));
    QVERIFY(!variables.value(QStringLiteral("includeCatalog")).toBool());
    QVERIFY(variables.value(QStringLiteral("includeUserList")).toBool());
}

void AniListGraphQlParsingTests::graphQlAdapterRejectsNonBooleanUserListPaginationMetadata() {
    auto payload = fixture(QStringLiteral(HAIKENANIME_GRAPHQL_USERLIST_FIXTURE));
    QVERIFY(payload.contains("\"hasNextChunk\": false"));
    payload.replace("\"hasNextChunk\": false", "\"hasNextChunk\": \"false\"");
    LocalGraphQlServer server;
    QVERIFY2(server.start(payload), "Local GraphQL server did not start.");
    QNetworkAccessManager networkManager;
    AniListGraphQlClient client(networkManager, nullptr, server.endpoint(), 1000);
    GraphQlQueryStore store(QStringLiteral(":/anilist/queries/media-page.graphql"));
    GraphQlAniListDataSource source(client, store);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.username = QStringLiteral("fixture-user");
    AniListDataSourceResult result;
    QString error;

    QVERIFY(!source.fetchPage(request, result, error));
    QVERIFY(error.contains(QStringLiteral("hasNextChunk")));
    QVERIFY(!result.isCompleteAuthoritativeSnapshot);
}

QTEST_MAIN(AniListGraphQlParsingTests)
#include "AniListGraphQlParsingTests.moc"
