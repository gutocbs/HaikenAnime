#include <QDir>
#include <QtTest>

#include <QMap>

#include "../../src/application/anilist/IAniListDataSource.h"
#include "../../src/application/anilist/AniListSyncService.h"
#include "../../src/application/media/MediaSyncFilter.h"
#include "../../src/application/media/MediaPage.h"
#include "../../src/application/media/IMediaSnapshotReconciler.h"
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

class CancellationAfterFetchDataSource final : public IMediaDataSource {
public:
    explicit CancellationAfterFetchDataSource(bool &cancelled) : cancelled_(cancelled) {}

    bool fetchPage(const MediaSyncFilter &, MediaPage &page, QString &error) override {
        error.clear();
        page = {};
        page.currentPage = 1;
        page.hasNextPage = false;
        Media media;
        media.Id = 7;
        page.media.append(media);
        cancelled_ = true;
        return true;
    }

private:
    bool &cancelled_;
};

class RecordingSnapshotReconciler final : public IMediaSnapshotReconciler {
public:
    bool reconcileAuthoritativeSnapshot(const QSet<int> &ids, int &removedCount,
                                        QString &error) override {
        ++calls;
        observedIds = ids;
        removedCount = 0;
        error.clear();
        return succeeds;
    }

    int calls = 0;
    QSet<int> observedIds;
    bool succeeds = true;
};

class IdempotentWriter final : public IMediaWriter {
public:
    bool upsert(const QList<Media> &media, QString &error) override {
        ++upsertCalls;
        if (!succeeds) {
            error = QStringLiteral("writer persistence failed");
            return false;
        }
        for (const auto &item : media) persistedById.insert(item.Id, item);
        error.clear();
        return true;
    }

    QMap<int, Media> persistedById;
    int upsertCalls = 0;
    bool succeeds = true;
};

class PartitionResultDataSource final : public IAniListDataSource {
public:
    bool fetchPage(const AniListDataSourceRequest &request, AniListDataSourceResult &result,
                   QString &error) override {
        requests.append(request);
        result = nextResult;
        error.clear();
        return true;
    }

    AniListDataSourceResult nextResult;
    QList<AniListDataSourceRequest> requests;
};

class PagedPartitionResultDataSource final : public IAniListDataSource {
public:
    bool fetchPage(const AniListDataSourceRequest &request, AniListDataSourceResult &result,
                   QString &error) override {
        requests.append(request);
        if (!resultsByPage.contains(request.filter.startingPage)) {
            error = QStringLiteral("No result configured for page %1.").arg(request.filter.startingPage);
            return false;
        }
        result = resultsByPage.value(request.filter.startingPage);
        error.clear();
        return true;
    }

    QMap<int, AniListDataSourceResult> resultsByPage;
    QList<AniListDataSourceRequest> requests;
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
    void legacyDataSourceDoesNotReconcileUnseenMedia();
    void filteredSynchronizationDoesNotReconcile();
    void synchronizationStartingAfterPageOneDoesNotReconcile();
    void replayAfterCheckpointFailureIsIdempotentForLegacySource();
    void doesNotAdvanceCheckpointWhenPagePersistenceFails();
    void cancellationAfterFetchPreventsPagePersistence();
    void partitionRequestsUseExplicitCatalogVariables();
    void partialPartitionDoesNotReconcileUnseenMedia();
    void reconcilesFullMultiPageUserListAtTerminalPage();
    void partialUserListRunDoesNotReconcileAtTerminalPage();
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
    QCOMPARE(page.media.first().ListStatus, UserListStatus::Current);
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

    QVERIFY(service.synchronize(MediaSyncFilter {}, error));
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

void AniListFlowTests::legacyDataSourceDoesNotReconcileUnseenMedia() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    MediaSyncFilter filter;
    filter.perPage = 2;
    QString error;

    QVERIFY2(service.synchronize(filter, error), qPrintable(error));
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::filteredSynchronizationDoesNotReconcile() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    MediaSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;
    QString error;

    QVERIFY2(service.synchronize(filter, error), qPrintable(error));
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::synchronizationStartingAfterPageOneDoesNotReconcile() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    MediaSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;
    filter.startingPage = 2;
    QString error;

    QVERIFY2(service.synchronize(filter, error), qPrintable(error));
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::replayAfterCheckpointFailureIsIdempotentForLegacySource() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    IdempotentWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    MediaSyncFilter filter;
    filter.type = QStringLiteral("TV");
    filter.perPage = 1;
    QString error;
    int failedCheckpointCalls = 0;

    QVERIFY(!service.synchronize(filter, error, [&](const int page, QString &checkpointError) {
        ++failedCheckpointCalls;
        if (page == 1) {
            checkpointError = QStringLiteral("checkpoint persistence interrupted");
            return false;
        }
        return true;
    }));
    QCOMPARE(failedCheckpointCalls, 1);
    QCOMPARE(writer.persistedById.size(), 1);
    QCOMPARE(reconciler.calls, 0);

    QList<int> confirmedPages;
    QVERIFY2(service.synchronize(filter, error, [&](const int page, QString &checkpointError) {
        confirmedPages.append(page);
        checkpointError.clear();
        return true;
    }), qPrintable(error));
    QCOMPARE(confirmedPages, QList<int>({1, 2}));
    QCOMPARE(writer.persistedById.size(), 2);
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::doesNotAdvanceCheckpointWhenPagePersistenceFails() {
    FileAniListDataSource source(QDir::cleanPath(fixturePath()));
    IdempotentWriter writer;
    writer.succeeds = false;
    AniListSyncService service(source, writer);
    QString error;
    int checkpointCalls = 0;

    QVERIFY(!service.synchronize(MediaSyncFilter {}, error, [&](const int, QString &) {
        ++checkpointCalls;
        return true;
    }));
    QCOMPARE(checkpointCalls, 0);
}

void AniListFlowTests::cancellationAfterFetchPreventsPagePersistence() {
    bool cancelled = false;
    CancellationAfterFetchDataSource source(cancelled);
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    QString error;

    QVERIFY(!service.synchronize(MediaSyncFilter {}, error, {}, [&cancelled] { return cancelled; }));
    QCOMPARE(service.lastErrorCategory(), AniListSyncErrorCategory::Cancelled);
    QCOMPARE(writer.batches.size(), 0);
}

void AniListFlowTests::partitionRequestsUseExplicitCatalogVariables() {
    const auto userList = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    const auto active = AniListDataSourceRequest::ForPartition(SyncPartition::ActiveCatalog);
    const auto inactive = AniListDataSourceRequest::ForPartition(SyncPartition::InactiveCatalog);
    const auto completed = AniListDataSourceRequest::ForPartition(SyncPartition::CompletedCatalog);

    QCOMPARE(userList.queryIdentity, QStringLiteral("user-list:v1"));
    QVERIFY(userList.queryIdentity != active.queryIdentity);
    QVERIFY(userList.variables.value(QStringLiteral("type")).isNull());
    QVERIFY(userList.variables.value(QStringLiteral("status")).isNull());
    QVERIFY(userList.variables.value(QStringLiteral("list")).isNull());

    QCOMPARE(active.queryIdentity, QStringLiteral("catalog:v1"));
    QCOMPARE(active.variables.value(QStringLiteral("type")).toString(), QStringLiteral("ANIME"));
    QCOMPARE(active.variables.value(QStringLiteral("status")).toString(), QStringLiteral("RELEASING"));
    QCOMPARE(inactive.variables.value(QStringLiteral("status")).toString(), QStringLiteral("NOT_YET_RELEASED"));
    QCOMPARE(completed.variables.value(QStringLiteral("status")).toString(), QStringLiteral("FINISHED"));
    QVERIFY(active.variables.value(QStringLiteral("list")).isNull());
    QVERIFY(inactive.variables.value(QStringLiteral("list")).isNull());
    QVERIFY(completed.variables.value(QStringLiteral("list")).isNull());
}

void AniListFlowTests::partialPartitionDoesNotReconcileUnseenMedia() {
    PartitionResultDataSource source;
    source.nextResult.completedPartition = SyncPartition::ActiveCatalog;
    source.nextResult.isCompleteAuthoritativeSnapshot = false;
    source.nextResult.page.currentPage = 1;
    Media media;
    media.Id = 154587;
    source.nextResult.page.media.append(media);
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    const auto request = AniListDataSourceRequest::ForPartition(SyncPartition::ActiveCatalog);
    QString error;

    QVERIFY2(service.synchronize(request, error), qPrintable(error));
    QCOMPARE(source.requests.size(), 1);
    QCOMPARE(source.requests.first().filter.partition, SyncPartition::ActiveCatalog);
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::reconcilesFullMultiPageUserListAtTerminalPage() {
    PagedPartitionResultDataSource source;
    source.resultsByPage[1].completedPartition = SyncPartition::UserList;
    source.resultsByPage[1].isCompleteAuthoritativeSnapshot = true;
    source.resultsByPage[1].page.currentPage = 1;
    source.resultsByPage[1].page.hasNextPage = true;
    source.resultsByPage[1].page.media.append(Media { .Id = 101 });
    source.resultsByPage[2].completedPartition = SyncPartition::UserList;
    source.resultsByPage[2].page.currentPage = 2;
    source.resultsByPage[2].page.media.append(Media { .Id = 202 });
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.username = QStringLiteral("fixture-user");
    QString error;

    QVERIFY2(service.synchronize(request, error), qPrintable(error));
    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(source.requests.at(0).filter.startingPage, 1);
    QCOMPARE(source.requests.at(1).filter.startingPage, 2);
    QCOMPARE(reconciler.calls, 1);
    QCOMPARE(reconciler.observedIds, QSet<int>({101, 202}));
}

void AniListFlowTests::partialUserListRunDoesNotReconcileAtTerminalPage() {
    PagedPartitionResultDataSource source;
    source.resultsByPage[2].completedPartition = SyncPartition::UserList;
    source.resultsByPage[2].isCompleteAuthoritativeSnapshot = true;
    source.resultsByPage[2].page.currentPage = 2;
    source.resultsByPage[2].page.media.append(Media { .Id = 202 });
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.username = QStringLiteral("fixture-user");
    request.setPage(2);
    QString error;

    QVERIFY2(service.synchronize(request, error), qPrintable(error));
    QCOMPARE(reconciler.calls, 0);
}

QTEST_MAIN(AniListFlowTests)
#include "AniListFlowTests.moc"
