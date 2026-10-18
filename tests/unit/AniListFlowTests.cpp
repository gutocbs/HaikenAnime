#include <QtTest>

#include <QMap>

#include "../../src/application/anilist/IAniListDataSource.h"
#include "../../src/application/anilist/AniListSyncService.h"
#include "../../src/application/anilist/AniListPendingChangeReconciler.h"
#include "../../src/application/anilist/AniListSnapshotReconciliation.h"
#include "../../src/application/media/MediaSyncFilter.h"
#include "../../src/application/media/MediaPage.h"
#include "../../src/application/media/IMediaSnapshotReconciler.h"
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

class WrongPageDataSource final : public IAniListDataSource {
public:
    bool fetchPage(const AniListDataSourceRequest &, AniListDataSourceResult &result, QString &error) override {
        error.clear();
        result = {};
        result.page.currentPage = 1;
        result.page.hasNextPage = true;
        return true;
    }
};

class CancellationAfterFetchDataSource final : public IAniListDataSource {
public:
    explicit CancellationAfterFetchDataSource(bool &cancelled) : cancelled_(cancelled) {}

    bool fetchPage(const AniListDataSourceRequest &, AniListDataSourceResult &result, QString &error) override {
        error.clear();
        result = {};
        result.page.currentPage = 1;
        result.page.hasNextPage = false;
        Media media;
        media.Id = 7;
        result.page.media.append(media);
        cancelled_ = true;
        return true;
    }

private:
    bool &cancelled_;
};

class RecordingSnapshotReconciler final : public IMediaSnapshotReconciler {
public:
    bool reconcileAuthoritativeSnapshot(const QSet<int> &ids, const QSet<int> &excluded,
                                        int &removedCount, QString &error) override {
        ++calls;
        observedIds = ids;
        excludedIds = excluded;
        removedCount = 0;
        error.clear();
        return succeeds;
    }

    int calls = 0;
    QSet<int> observedIds;
    QSet<int> excludedIds;
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

class ReconciliationPendingRepository final : public IPendingChangeRepository {
public:
    bool enqueue(const AniListPendingChange &change, QString &error) override {
        pending = change;
        error.clear();
        return true;
    }
    bool getPending(const int mediaId, QList<AniListPendingChange> &result,
                    QString &error) override {
        result.clear();
        if (pending.mediaId == mediaId
            && pending.status != AniListPendingChangeStatus::Succeeded
            && pending.status != AniListPendingChangeStatus::Superseded) {
            result.append(pending);
        }
        error.clear();
        return true;
    }
    bool getMediaIdsWithUnsentChanges(QSet<int> &result, QString &error) override {
        result = unsentIds;
        error.clear();
        return true;
    }
    bool getPendingMediaIds(QList<int> &result, QString &error) override {
        result.clear();
        error.clear();
        return true;
    }
    bool updateStatus(const AniListPendingChange &change, QString &error) override {
        pending = change;
        error.clear();
        return true;
    }

    AniListPendingChange pending;
    QSet<int> unsentIds;
};

class CancelAfterFirstChunkDataSource final : public IAniListDataSource {
public:
    explicit CancelAfterFirstChunkDataSource(bool &cancelled) : cancelled_(cancelled) {}

    bool fetchPage(const AniListDataSourceRequest &request, AniListDataSourceResult &result,
                   QString &error) override {
        result = {};
        result.completedPartition = SyncPartition::UserList;
        result.isCompleteAuthoritativeSnapshot = request.filter.startingPage == 1;
        result.page.currentPage = request.filter.startingPage;
        result.page.hasNextPage = request.filter.startingPage == 1;
        result.observedMediaIds = QSet<int>({101});
        if (request.filter.startingPage == 1) cancelled_ = true;
        error.clear();
        return true;
    }

private:
    bool &cancelled_;
};

AniListSyncOutcome CompleteOutcome(const QSet<int> &ids) {
    return {.observedMediaIds = ids, .completeSnapshot = true};
}

/** Two user-list pages with media 1 and 2, the shape the sync service sees from AniList. */
void ConfigureTwoPages(PagedPartitionResultDataSource &source) {
    source.resultsByPage[1].page.currentPage = 1;
    source.resultsByPage[1].page.hasNextPage = true;
    source.resultsByPage[1].page.media.append(Media {.Id = 1});
    source.resultsByPage[2].page.currentPage = 2;
    source.resultsByPage[2].page.media.append(Media {.Id = 2});
}

class AniListFlowTests : public QObject {
    Q_OBJECT

private slots:
    void synchronizationPersistsEveryPage();
    void mapperUsesNeutralValuesForUnknownExternalEnums();
    void successfulSynchronizationClearsPreviousError();
    void rejectsInvalidPaginationBeforeReading();
    void rejectsDataSourceThatDoesNotReturnRequestedPage();
    void replayAfterCheckpointFailureIsIdempotent();
    void doesNotAdvanceCheckpointWhenPagePersistenceFails();
    void cancellationAfterFetchPreventsPagePersistence();
    void partitionRequestsUseExplicitCatalogVariables();
    void partialPartitionDoesNotReconcileUnseenMedia();
    void reconcilesFullMultiPageUserListAtTerminalPage();
    void partialUserListRunDoesNotReconcileAtTerminalPage();
    void typedAndStatusFilteredFullCollectionReportsCompleteSnapshot();
    void failureOnSecondChunkLeavesSnapshotIncomplete();
    void nonAuthoritativeFirstChunkLeavesSnapshotIncomplete();
    void reconcilesPendingProgressBeforePersistingTheRemotePage();
    void completeAnimeAndMangaReconcileUnionExcludingUnsentMedia();
    void failedOrMissingTypeOutcomeNeverReconciles();
    void emptySnapshotSkipsReconciliationAndWarns();
    void cancellationBetweenTypesNeverReconciles();
    void droppedMediaObservedWhileListDisabledIsNotReported();
    void cancellationBetweenChunksLeavesSnapshotIncomplete();
    void reportsCumulativeMediaCountAfterEachPersistedPage();
};

void AniListFlowTests::synchronizationPersistsEveryPage() {
    PagedPartitionResultDataSource source;
    ConfigureTwoPages(source);
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    QString error;

    QVERIFY2(service.synchronize(AniListDataSourceRequest::ForPartition(SyncPartition::UserList), error),
             qPrintable(error));
    QCOMPARE(writer.batches.size(), 2);
    QCOMPARE(writer.batches.at(0).first().Id, 1);
    QCOMPARE(writer.batches.at(1).first().Id, 2);
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
    PagedPartitionResultDataSource source;
    ConfigureTwoPages(source);
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    QString error = QStringLiteral("stale error");

    QVERIFY(service.synchronize(AniListDataSourceRequest::ForPartition(SyncPartition::UserList), error));
    QVERIFY(error.isEmpty());
}

void AniListFlowTests::rejectsInvalidPaginationBeforeReading() {
    WrongPageDataSource source;
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.perPage = 0;
    QString error;

    QVERIFY(!service.synchronize(request, error));
    QCOMPARE(service.lastErrorCategory(), AniListSyncErrorCategory::InvalidData);
    QVERIFY(error.contains(QStringLiteral("positive")));
}

void AniListFlowTests::rejectsDataSourceThatDoesNotReturnRequestedPage() {
    WrongPageDataSource source;
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.setPage(2);
    QString error;

    QVERIFY(!service.synchronize(request, error));
    QCOMPARE(service.lastErrorCategory(), AniListSyncErrorCategory::InvalidData);
    QVERIFY(error.contains(QStringLiteral("page 1")));
}

void AniListFlowTests::replayAfterCheckpointFailureIsIdempotent() {
    PagedPartitionResultDataSource source;
    ConfigureTwoPages(source);
    IdempotentWriter writer;
    AniListSyncService service(source, writer);
    const auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    QString error;
    int failedCheckpointCalls = 0;

    QVERIFY(!service.synchronize(request, error, [&](const int page, QString &checkpointError) {
        ++failedCheckpointCalls;
        if (page == 1) {
            checkpointError = QStringLiteral("checkpoint persistence interrupted");
            return false;
        }
        return true;
    }));
    QCOMPARE(failedCheckpointCalls, 1);
    QCOMPARE(writer.persistedById.size(), 1);

    QList<int> confirmedPages;
    QVERIFY2(service.synchronize(request, error, [&](const int page, QString &checkpointError) {
        confirmedPages.append(page);
        checkpointError.clear();
        return true;
    }), qPrintable(error));
    QCOMPARE(confirmedPages, QList<int>({1, 2}));
    QCOMPARE(writer.persistedById.size(), 2);
}

void AniListFlowTests::doesNotAdvanceCheckpointWhenPagePersistenceFails() {
    PagedPartitionResultDataSource source;
    ConfigureTwoPages(source);
    IdempotentWriter writer;
    writer.succeeds = false;
    AniListSyncService service(source, writer);
    QString error;
    int checkpointCalls = 0;

    QVERIFY(!service.synchronize(AniListDataSourceRequest::ForPartition(SyncPartition::UserList), error,
                                 [&](const int, QString &) {
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

    QVERIFY(!service.synchronize(AniListDataSourceRequest::ForPartition(SyncPartition::UserList), error, {},
                                 [&cancelled] { return cancelled; }));
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

void AniListFlowTests::typedAndStatusFilteredFullCollectionReportsCompleteSnapshot() {
    PagedPartitionResultDataSource source;
    source.resultsByPage[1].completedPartition = SyncPartition::UserList;
    source.resultsByPage[1].isCompleteAuthoritativeSnapshot = true;
    source.resultsByPage[1].page.currentPage = 1;
    source.resultsByPage[1].page.hasNextPage = true;
    source.resultsByPage[1].page.media.append(Media { .Id = 101 });
    source.resultsByPage[1].observedMediaIds = QSet<int>({101, 102});
    source.resultsByPage[2].completedPartition = SyncPartition::UserList;
    source.resultsByPage[2].page.currentPage = 2;
    source.resultsByPage[2].page.media.append(Media { .Id = 202 });
    source.resultsByPage[2].observedMediaIds = QSet<int>({202, 203});
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.type = QStringLiteral("ANIME");
    request.filter.acceptedListStatuses = {QStringLiteral("CURRENT")};
    QString error;

    QVERIFY2(service.synchronize(request, error), qPrintable(error));

    QVERIFY(service.lastOutcome().completeSnapshot);
    QCOMPARE(service.lastOutcome().observedMediaIds, QSet<int>({101, 102, 202, 203}));
    QCOMPARE(reconciler.calls, 1);
    QCOMPARE(reconciler.observedIds, QSet<int>({101, 102, 202, 203}));
}

void AniListFlowTests::failureOnSecondChunkLeavesSnapshotIncomplete() {
    PagedPartitionResultDataSource source;
    source.resultsByPage[1].completedPartition = SyncPartition::UserList;
    source.resultsByPage[1].isCompleteAuthoritativeSnapshot = true;
    source.resultsByPage[1].page.currentPage = 1;
    source.resultsByPage[1].page.hasNextPage = true;
    source.resultsByPage[1].observedMediaIds = QSet<int>({101});
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.type = QStringLiteral("ANIME");
    QString error;

    QVERIFY(!service.synchronize(request, error));

    QVERIFY(!service.lastOutcome().completeSnapshot);
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::nonAuthoritativeFirstChunkLeavesSnapshotIncomplete() {
    PagedPartitionResultDataSource source;
    source.resultsByPage[1].completedPartition = SyncPartition::UserList;
    source.resultsByPage[1].isCompleteAuthoritativeSnapshot = false;
    source.resultsByPage[1].page.currentPage = 1;
    source.resultsByPage[1].observedMediaIds = QSet<int>({101});
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.status = QStringLiteral("CURRENT");
    QString error;

    QVERIFY2(service.synchronize(request, error), qPrintable(error));

    QVERIFY(!service.lastOutcome().completeSnapshot);
    QCOMPARE(service.lastOutcome().observedMediaIds, QSet<int>({101}));
}

void AniListFlowTests::reconcilesPendingProgressBeforePersistingTheRemotePage() {
    PartitionResultDataSource source;
    source.nextResult.completedPartition = SyncPartition::UserList;
    source.nextResult.page.currentPage = 1;
    source.nextResult.page.media.append(Media{.Id = 42, .ConsumedChapters = 12});
    ReconciliationPendingRepository pendingRepository;
    pendingRepository.pending.id = 1;
    pendingRepository.pending.mediaId = 42;
    pendingRepository.pending.field = AniListField::Progress;
    pendingRepository.pending.previousValue = 9;
    pendingRepository.pending.newValue = 10;
    AniListPendingChangeReconciler pendingReconciler(pendingRepository);
    CollectingWriter writer;
    AniListSyncService service(source, writer, nullptr, nullptr, 0, &pendingReconciler);
    QString error;

    QVERIFY2(service.synchronize(AniListDataSourceRequest::ForPartition(SyncPartition::UserList),
                                 error), qPrintable(error));
    QCOMPARE(writer.batches.size(), 1);
    QCOMPARE(writer.batches.first().first().ConsumedChapters, 12);
    QCOMPARE(pendingRepository.pending.status, AniListPendingChangeStatus::Superseded);
}

void AniListFlowTests::completeAnimeAndMangaReconcileUnionExcludingUnsentMedia() {
    RecordingSnapshotReconciler reconciler;
    ReconciliationPendingRepository pending;
    pending.unsentIds = QSet<int>({9});
    bool reconciled = false;
    int removedCount = -1;
    QString error;

    QVERIFY2(ReconcileCompleteAniListSnapshot(
                 {CompleteOutcome({1, 2}), CompleteOutcome({3})}, 2, true, reconciler, pending,
                 reconciled, removedCount, error), qPrintable(error));

    QVERIFY(reconciled);
    QCOMPARE(reconciler.calls, 1);
    QCOMPARE(reconciler.observedIds, QSet<int>({1, 2, 3}));
    QCOMPARE(reconciler.excludedIds, QSet<int>({9}));
}

void AniListFlowTests::emptySnapshotSkipsReconciliationAndWarns() {
    RecordingSnapshotReconciler reconciler;
    ReconciliationPendingRepository pending;
    bool reconciled = true;
    int removedCount = -1;
    QString error;
    QStringList logs;

    QVERIFY2(ReconcileCompleteAniListSnapshot(
                 {CompleteOutcome({}), CompleteOutcome({})}, 2, true, reconciler, pending,
                 reconciled, removedCount, error,
                 [&logs](const QString &line) { logs.append(line); }), qPrintable(error));

    QVERIFY(!reconciled);
    QCOMPARE(removedCount, 0);
    QCOMPARE(reconciler.calls, 0);
    QCOMPARE(logs, QStringList({QStringLiteral(
        "AniList snapshot reconciliation skipped: snapshot contained no entries.")}));
}

void AniListFlowTests::failedOrMissingTypeOutcomeNeverReconciles() {
    RecordingSnapshotReconciler reconciler;
    ReconciliationPendingRepository pending;
    bool reconciled = true;
    int removedCount = 0;
    QString error;

    QVERIFY(ReconcileCompleteAniListSnapshot(
        {CompleteOutcome({1})}, 2, false, reconciler, pending, reconciled, removedCount, error));
    QVERIFY(!reconciled);
    QVERIFY(ReconcileCompleteAniListSnapshot(
        {CompleteOutcome({1})}, 2, true, reconciler, pending, reconciled, removedCount, error));
    QVERIFY(!reconciled);
    QVERIFY(ReconcileCompleteAniListSnapshot(
        {CompleteOutcome({1}), AniListSyncOutcome {.observedMediaIds = {2}}}, 2, true, reconciler,
        pending, reconciled, removedCount, error));
    QVERIFY(!reconciled);
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::cancellationBetweenTypesNeverReconciles() {
    RecordingSnapshotReconciler reconciler;
    ReconciliationPendingRepository pending;
    bool reconciled = true;
    int removedCount = 0;
    QString error;

    QVERIFY(ReconcileCompleteAniListSnapshot(
        {CompleteOutcome({1}), CompleteOutcome({2})}, 2, false, reconciler, pending, reconciled,
        removedCount, error));

    QVERIFY(!reconciled);
    QCOMPARE(reconciler.calls, 0);
}

void AniListFlowTests::droppedMediaObservedWhileListDisabledIsNotReported() {
    RecordingSnapshotReconciler reconciler;
    ReconciliationPendingRepository pending;
    bool reconciled = false;
    int removedCount = 0;
    QString error;
    constexpr int droppedMediaId = 55;

    QVERIFY(ReconcileCompleteAniListSnapshot(
        {CompleteOutcome({1, droppedMediaId}), CompleteOutcome({})}, 2, true, reconciler, pending,
        reconciled, removedCount, error));

    QVERIFY(reconciler.observedIds.contains(droppedMediaId));
}

void AniListFlowTests::cancellationBetweenChunksLeavesSnapshotIncomplete() {
    bool cancelled = false;
    CancelAfterFirstChunkDataSource source(cancelled);
    CollectingWriter writer;
    RecordingSnapshotReconciler reconciler;
    AniListSyncService service(source, writer, &reconciler);
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    request.filter.type = QStringLiteral("ANIME");
    QString error;

    QVERIFY(!service.synchronize(request, error, {}, [&cancelled] { return cancelled; }));

    QCOMPARE(service.lastErrorCategory(), AniListSyncErrorCategory::Cancelled);
    QVERIFY(!service.lastOutcome().completeSnapshot);
    QCOMPARE(reconciler.calls, 0);
}

QTEST_MAIN(AniListFlowTests)
#include "AniListFlowTests.moc"

void AniListFlowTests::reportsCumulativeMediaCountAfterEachPersistedPage() {
    PagedPartitionResultDataSource source;
    source.resultsByPage[1].completedPartition = SyncPartition::UserList;
    source.resultsByPage[1].page.currentPage = 1;
    source.resultsByPage[1].page.hasNextPage = true;
    for (const int id : {1, 2, 3}) source.resultsByPage[1].page.media.append(Media { .Id = id });
    source.resultsByPage[2].completedPartition = SyncPartition::UserList;
    source.resultsByPage[2].page.currentPage = 2;
    for (const int id : {4, 5}) source.resultsByPage[2].page.media.append(Media { .Id = id });
    CollectingWriter writer;
    AniListSyncService service(source, writer);
    QList<int> reported;
    service.setProgressReporter([&reported](const int processedItems) { reported.append(processedItems); });
    auto request = AniListDataSourceRequest::ForPartition(SyncPartition::UserList);
    QString error;

    QVERIFY2(service.synchronize(request, error), qPrintable(error));

    QCOMPARE(reported, QList<int>({3, 5}));
}
