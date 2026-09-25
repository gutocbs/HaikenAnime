#include <QtTest>

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
                    "title": {
                        "romaji": "Sousou no Frieren",
                        "english": "Frieren: Beyond Journey's End",
                        "native": "葬送のフリーレン"
                    },
                    "synonyms": ["Frieren at the Funeral"],
                    "episodes": null,
                    "chapters": 64,
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

QTEST_MAIN(AniListGraphQlParsingTests)
#include "AniListGraphQlParsingTests.moc"
