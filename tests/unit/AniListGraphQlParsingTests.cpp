#include <QtTest>
#include <QFile>

#include "../../src/infrastructure/anilist/AniListGraphQlPageParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlResponseParser.h"
#include "../../src/infrastructure/anilist/AniListMediaMapper.h"

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
    void recordedLibraryFixtureIsComplete();
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

QTEST_MAIN(AniListGraphQlParsingTests)
#include "AniListGraphQlParsingTests.moc"
