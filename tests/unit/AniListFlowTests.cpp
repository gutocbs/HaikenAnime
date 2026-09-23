#include <QCoreApplication>
#include <QDir>
#include <QtTest>

#include "../../src/application/anilist/AniListSyncService.h"
#include "../../src/infrastructure/anilist/FileAniListDataSource.h"

class CollectingRepository final : public IMediaRepository {
public:
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
};

static QString fixturePath() {
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("../tests/fixtures/media-library.json"));
}

void AniListFlowTests::fixtureAppliesFilterAndPagination() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    AniListSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;

    AniListPage page;
    QString error;
    QVERIFY(source.fetchPage(filter, page, error));
    QCOMPARE(page.externalMedia.size(), 1);
    QCOMPARE(page.currentPage, 1);
    QVERIFY(page.hasNextPage);
    QCOMPARE(page.externalMedia.first().id, 154587);
}

void AniListFlowTests::synchronizationPersistsEveryPage() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingRepository repository;
    AniListSyncService service(source, repository);
    AniListSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;
    QString error;

    QVERIFY(service.Synchronize(filter, error));
    QCOMPARE(repository.batches.size(), 2);
    QCOMPARE(repository.batches.at(0).first().Id, 154587);
    QCOMPARE(repository.batches.at(1).first().Id, 116807);
}

QTEST_MAIN(AniListFlowTests)
#include "AniListFlowTests.moc"
