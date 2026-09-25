#include <QDir>
#include <QtTest>

#include "../../src/application/anilist/AniListSyncService.h"
#include "../../src/application/media/MediaSyncFilter.h"
#include "../../src/application/media/MediaPage.h"
#include "../../src/infrastructure/anilist/FileAniListDataSource.h"
#include "../../src/infrastructure/anilist/AniListMediaMapper.h"

class CollectingWriter final : public IMediaWriter {
public:
    bool upsert(const QList<Media> &media, QString &error) override {
        error.clear();
        batches.append(media);
        return true;
    }

    QList<QList<Media>> batches;
};

class WrongPageDataSource final : public IMediaDataSource {
public:
    bool fetchPage(const MediaSyncFilter &, MediaPage &page, QString &error) override {
        error.clear();
        page = {};
        page.currentPage = 1;
        page.hasNextPage = true;
        return true;
    }
};

class AniListFlowTests : public QObject {
    Q_OBJECT

private slots:
    void fixtureAppliesFilterAndPagination();
    void embeddedProductionFixtureCanBeRead();
    void synchronizationPersistsEveryPage();
    void invalidFixtureReturnsError();
    void mapperUsesNeutralValuesForUnknownExternalEnums();
    void successfulSynchronizationClearsPreviousError();
    void rejectsInvalidPaginationBeforeReading();
    void rejectsDataSourceThatDoesNotReturnRequestedPage();
};

static QString fixturePath() {
    return QDir(QStringLiteral(HAIKENANIME_TEST_FIXTURE_DIR))
        .filePath(QStringLiteral("media-library.json"));
}

void AniListFlowTests::fixtureAppliesFilterAndPagination() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    MediaSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;

    MediaPage page;
    QString error;
    QVERIFY(source.fetchPage(filter, page, error));
    QCOMPARE(page.media.size(), 1);
    QCOMPARE(page.currentPage, 1);
    QVERIFY(page.hasNextPage);
    QCOMPARE(page.media.first().Id, 154587);
}

void AniListFlowTests::embeddedProductionFixtureCanBeRead() {
    FileAniListDataSource source(QStringLiteral(":/fixtures/media-library.json"));
    MediaPage page;
    QString error;

    QVERIFY2(source.fetchPage({}, page, error), qPrintable(error));
    QVERIFY(!page.media.isEmpty());
}

void AniListFlowTests::synchronizationPersistsEveryPage() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    MediaSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;
    QString error;

    QVERIFY(service.synchronize(filter, error));
    QCOMPARE(writer.batches.size(), 2);
    QCOMPARE(writer.batches.at(0).first().Id, 154587);
    QCOMPARE(writer.batches.at(1).first().Id, 116807);
}

void AniListFlowTests::invalidFixtureReturnsError() {
    FileAniListDataSource source(QDir(QStringLiteral(HAIKENANIME_TEST_FIXTURE_DIR))
                                     .filePath(QStringLiteral("invalid-media-library.json")));
    MediaPage page;
    QString error;
    QVERIFY(!source.fetchPage({}, page, error));
    QVERIFY(error.contains(QStringLiteral("Invalid AniList fixture")));
}

void AniListFlowTests::mapperUsesNeutralValuesForUnknownExternalEnums() {
    AniListMediaDto externalMedia;
    externalMedia.id = 42;
    externalMedia.mediaType = QStringLiteral("UNSUPPORTED_TYPE");
    externalMedia.status = QStringLiteral("CANCELLED");

    const auto media = AniListMediaMapper::ToDomainMedia(externalMedia);

    QCOMPARE(media.Type, MediaType::Unknown);
    QCOMPARE(media.Status, MediaStatus::Unknown);
    QCOMPARE(media.ConsumedChapters, 0);
    QCOMPARE(media.NextChapter, 0);
    QCOMPARE(media.PersonalScore, 0);
}

void AniListFlowTests::successfulSynchronizationClearsPreviousError() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    QString error = QStringLiteral("stale error");

    QVERIFY(service.synchronize({}, error));
    QVERIFY(error.isEmpty());
}

void AniListFlowTests::rejectsInvalidPaginationBeforeReading() {
    WrongPageDataSource source;
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    MediaSyncFilter filter;
    filter.perPage = 0;
    QString error;

    QVERIFY(!service.synchronize(filter, error));
    QCOMPARE(service.lastErrorCategory(), AniListSyncErrorCategory::InvalidData);
    QVERIFY(error.contains(QStringLiteral("positive")));
}

void AniListFlowTests::rejectsDataSourceThatDoesNotReturnRequestedPage() {
    WrongPageDataSource source;
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    MediaSyncFilter filter;
    filter.startingPage = 2;
    QString error;

    QVERIFY(!service.synchronize(filter, error));
    QCOMPARE(service.lastErrorCategory(), AniListSyncErrorCategory::InvalidData);
    QVERIFY(error.contains(QStringLiteral("page 1")));
}

QTEST_MAIN(AniListFlowTests)
#include "AniListFlowTests.moc"
