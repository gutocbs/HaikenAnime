#include <QFile>
#include <QtTest>
#include <algorithm>

#include "../../src/infrastructure/anilist/AniListGraphQlResponseParser.h"
#include "../../src/infrastructure/anilist/AniListGraphQlUserListParser.h"

class AniListGraphQlUserListParserTests final : public QObject {
    Q_OBJECT
private slots:
    void parsesUserListFixture();
    void parsesFrierenProgressFromUserListFixture();
    void filtersEntriesByRequestedListStatus();
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

void AniListGraphQlUserListParserTests::filtersEntriesByRequestedListStatus() {
    QFile file(QStringLiteral(HAIKENANIME_USERLIST_FIXTURE));
    QVERIFY(file.open(QIODevice::ReadOnly));
    AniListGraphQlResponse response;
    QString error;
    QVERIFY2(AniListGraphQlResponseParser::parse(file.readAll(), response, error), qPrintable(error));

    QList<Media> allMedia;
    QVERIFY2(AniListGraphQlUserListParser::parse(response.data, allMedia, error), qPrintable(error));

    QList<Media> media;
    QVERIFY2(AniListGraphQlUserListParser::parse(response.data, media, error, QStringLiteral("CURRENT")),
             qPrintable(error));
    QVERIFY(!media.isEmpty());
    QVERIFY(media.size() < allMedia.size());
    QVERIFY(std::all_of(media.cbegin(), media.cend(), [](const Media &item) {
        return item.ListStatus == UserListStatus::Current;
    }));
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
