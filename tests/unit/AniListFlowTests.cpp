#include <QCoreApplication>
#include <QDir>
#include <QtTest>

#include "../../src/application/anilist/AniListSyncService.h"
#include "../../src/domain/media/MediaSyncFilter.h"
#include "../../src/domain/media/MediaPage.h"
#include "../../src/infrastructure/anilist/FileAniListDataSource.h"

class CollectingRepository final : public IMediaRepository {
public:
    QList<Media> ReadAll(QString &error) override {
        error.clear();
        return {};
    }

    bool Upsert(const QList<Media> &media, QString &error) override {
        Q_UNUSED(error)
        batches.append(media);
        return true;
    }

    QList<QList<Media>> batches;
};

class AniListFlowTests : public QObject {
    Q_OBJECT

private slots:
    void fixtureAppliesFilterAndPagination();
    void synchronizationPersistsEveryPage();
    void invalidFixtureReturnsError();
};

static QString fixturePath() {
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("../tests/fixtures/media-library.json"));
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

void AniListFlowTests::synchronizationPersistsEveryPage() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingRepository repository;
    AniListSyncService service(source, repository);
    MediaSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;
    QString error;

    QVERIFY(service.Synchronize(filter, error));
    QCOMPARE(repository.batches.size(), 2);
    QCOMPARE(repository.batches.at(0).first().Id, 154587);
    QCOMPARE(repository.batches.at(1).first().Id, 116807);
}

void AniListFlowTests::invalidFixtureReturnsError() {
    FileAniListDataSource source(QDir(QCoreApplication::applicationDirPath())
                                     .filePath(QStringLiteral("../tests/fixtures/invalid-media-library.json")));
    MediaPage page;
    QString error;
    QVERIFY(!source.fetchPage({}, page, error));
    QVERIFY(error.contains(QStringLiteral("Invalid AniList fixture")));
}

QTEST_MAIN(AniListFlowTests)
#include "AniListFlowTests.moc"
