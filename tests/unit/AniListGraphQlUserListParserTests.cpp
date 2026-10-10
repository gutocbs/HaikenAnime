#include <QFile>
#include <QJsonArray>
#include <QtTest>
#include <algorithm>

#include "../../src/infrastructure/anilist/AniListGraphQlResponseParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlUserListParser.h"

class AniListGraphQlUserListParserTests final : public QObject {
    Q_OBJECT
private slots:
    void parsesUserListFixture();
    void parsesFrierenProgressFromUserListFixture();
    void filtersEntriesByAcceptedListStatusesIncludingCustomLists();
    void keepsEveryNonNovelMangaFormatAndExcludesNovels();
    void rejectsMissingCollection();
};

void AniListGraphQlUserListParserTests::parsesUserListFixture() {
    QFile file(QStringLiteral(HAIKENANIME_USERLIST_FIXTURE));
    QVERIFY(file.open(QIODevice::ReadOnly));
    AniListGraphQlResponse response;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(file.readAll(), response, error), qPrintable(error));
    QList<Media> media;
    QVERIFY2(AniListGraphQlUserListParser::parse(response.data, media, error), qPrintable(error));
    QVERIFY(media.size() > 1);
    QCOMPARE(media.first().Id, 21366);
    QCOMPARE(media.first().ConsumedChapters, 22);
    QCOMPARE(media.first().ListStatus, UserListStatus::Completed);
    QCOMPARE(media.first().Type, MediaType::Anime);
}

void AniListGraphQlUserListParserTests::parsesFrierenProgressFromUserListFixture() {
    QFile file(QStringLiteral(HAIKENANIME_USERLIST_FIXTURE));
    QVERIFY(file.open(QIODevice::ReadOnly));
    AniListGraphQlResponse response;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(file.readAll(), response, error), qPrintable(error));

    QList<Media> media;
    QVERIFY2(AniListGraphQlUserListParser::parse(response.data, media, error), qPrintable(error));
    const auto matches = std::find_if(media.cbegin(), media.cend(), [](const Media &item) {
        return item.Id == 154587;
    });
    QVERIFY(matches != media.cend());
    QCOMPARE(matches->ConsumedChapters, 28);
    QCOMPARE(matches->TotalChapters, 28);
}

void AniListGraphQlUserListParserTests::filtersEntriesByAcceptedListStatusesIncludingCustomLists() {
    const auto entry = [](const int id, const QString &status) {
        return QJsonObject{
            {QStringLiteral("status"), status},
            {QStringLiteral("progress"), 1},
            {QStringLiteral("score"), 80},
            {QStringLiteral("media"), QJsonObject{
                {QStringLiteral("id"), id},
                {QStringLiteral("type"), QStringLiteral("ANIME")},
                {QStringLiteral("format"), QStringLiteral("TV")},
                {QStringLiteral("status"), QStringLiteral("RELEASING")},
                {QStringLiteral("title"), QJsonObject{
                    {QStringLiteral("romaji"), QStringLiteral("Title %1").arg(id)}}}
            }}
        };
    };
    const QJsonObject data{
        {QStringLiteral("MediaListCollection"), QJsonObject{
            {QStringLiteral("lists"), QJsonArray{
                QJsonObject{
                    {QStringLiteral("name"), QStringLiteral("Watching")},
                    {QStringLiteral("entries"), QJsonArray{entry(1, QStringLiteral("CURRENT"))}}
                },
                QJsonObject{
                    {QStringLiteral("name"), QStringLiteral("Favorites")},
                    {QStringLiteral("entries"), QJsonArray{
                        entry(2, QStringLiteral("COMPLETED")),
                        entry(3, QStringLiteral("DROPPED"))
                    }}
                }
            }}
        }}
    };
    QString error;
    QList<Media> media;
    QVERIFY2(AniListGraphQlUserListParser::parse(
                 data, media, error,
                 {QStringLiteral("CURRENT"), QStringLiteral("COMPLETED")}),
             qPrintable(error));
    QCOMPARE(media.size(), 2);
    QCOMPARE(media.at(0).Id, 1);
    QCOMPARE(media.at(0).ListStatus, UserListStatus::Current);
    QCOMPARE(media.at(1).Id, 2);
    QCOMPARE(media.at(1).ListStatus, UserListStatus::Completed);
}

void AniListGraphQlUserListParserTests::keepsEveryNonNovelMangaFormatAndExcludesNovels() {
    const auto entry = [](const int id, const QString &format) {
        return QJsonObject{
            {QStringLiteral("mediaId"), id},
            {QStringLiteral("status"), QStringLiteral("CURRENT")},
            {QStringLiteral("progress"), 1},
            {QStringLiteral("score"), 80},
            {QStringLiteral("media"), QJsonObject{
                {QStringLiteral("id"), id},
                {QStringLiteral("type"), QStringLiteral("MANGA")},
                {QStringLiteral("format"), format},
                {QStringLiteral("status"), QStringLiteral("RELEASING")},
                {QStringLiteral("title"), QJsonObject{
                    {QStringLiteral("romaji"), QStringLiteral("Title %1").arg(id)}}}
            }}
        };
    };
    const QJsonObject data{
        {QStringLiteral("MediaListCollection"), QJsonObject{
            {QStringLiteral("lists"), QJsonArray{
                QJsonObject{{QStringLiteral("entries"), QJsonArray{
                    entry(1, QStringLiteral("MANGA")),
                    entry(2, QStringLiteral("ONE_SHOT")),
                    entry(3, QStringLiteral("NOVEL"))
                }}}
            }}
        }}
    };
    QList<Media> media;
    QString error;

    QVERIFY2(AniListGraphQlUserListParser::parse(
                 data, media, error, {QStringLiteral("CURRENT")}, {MediaType::Manga}),
             qPrintable(error));
    QCOMPARE(media.size(), 2);
    QCOMPARE(media.at(0).Id, 1);
    QCOMPARE(media.at(0).Type, MediaType::Manga);
    QCOMPARE(media.at(1).Id, 2);
    QCOMPARE(media.at(1).Type, MediaType::Manga);
}

void AniListGraphQlUserListParserTests::rejectsMissingCollection() {
    QList<Media> media;
    QString error;
    QVERIFY(!AniListGraphQlUserListParser::parse(QJsonObject{}, media, error));
    QVERIFY(!error.isEmpty());
    QVERIFY(media.isEmpty());
}

QTEST_MAIN(AniListGraphQlUserListParserTests)
#include "AniListGraphQlUserListParserTests.moc"
