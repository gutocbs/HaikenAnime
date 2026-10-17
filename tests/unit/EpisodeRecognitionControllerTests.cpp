#include <QtTest>

#include "../../src/application/recognition/EpisodeRecognitionService.h"
#include "../../src/presentation/recognition/EpisodeRecognitionController.h"

namespace {
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

class MemoryHistory final : public IRecognitionHistoryRepository {
public:
    bool insert(RecognitionRecord &record, QString &error) override {
        error.clear();
        for (const auto &stored : records) {
            if (stored.deduplicationKey() == record.deduplicationKey()) return false;
        }
        record.assignId(records.size() + 1);
        records.append(record);
        return true;
    }
    bool update(const RecognitionRecord &record, QString &error) override {
        error.clear();
        for (auto &stored : records) {
            if (stored.id() == record.id()) stored = record;
        }
        return true;
    }
    bool find(const RecognitionHistoryQuery &query, QList<RecognitionRecord> &found, QString &error) override {
        error.clear();
        found.clear();
        for (auto record = records.crbegin(); record != records.crend(); ++record) {
            if (query.recordId && record->id() != *query.recordId) continue;
            if (!query.deduplicationKey.isEmpty() && record->deduplicationKey() != query.deduplicationKey) continue;
            if (query.beforeId && record->id() >= *query.beforeId) continue;
            if (found.size() >= query.limit) break;
            found.append(*record);
        }
        return true;
    }
    QList<RecognitionRecord> records;
};

class RecordingProgress final : public IRecognitionProgressWriter {
public:
    bool apply(const Media &, const Media &updated, QString &) override {
        applied.append(updated.ConsumedChapters);
        return true;
    }
    QList<int> applied;
};

class FakeLibrary final : public IMediaReader {
public:
    bool readAll(QList<Media> &media, QString &error) override {
        error.clear();
        media = items;
        return true;
    }
    QList<Media> items;
};

Media entry(const int id, const QString &name, const int consumed) {
    Media media;
    media.Id = id;
    media.Name = name;
    media.ConsumedChapters = consumed;
    media.ListStatus = UserListStatus::Current;
    return media;
}

PlaybackSession session(const QString &key) {
    return {.id = PlaybackSessionId::create(QStringLiteral("plex"), key).value(), .userId = QStringLiteral("1"),
            .deviceId = QStringLiteral("tv"), .startedAt = Start};
}
}

class EpisodeRecognitionControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void reloadListsNewestFirstWithLibraryTitles();
    void loadMorePagesWithCursor();
    void correctionAppliesCorrectedEpisode();
    void correctionRejectsInvalidEpisodeOrMedia();
    void ignoreMarksRecordIgnored();
    void mediaChoicesListTheLibrary();
    void modelExposesStatusAndCorrectability();

private:
    void addUncertain(const QString &key, int episode);

    MemoryHistory repository;
    RecordingProgress progress;
    FakeLibrary library;
    std::unique_ptr<RecognitionHistoryService> history;
    std::unique_ptr<MediaReaderProgressReader> reader;
    std::unique_ptr<ProgressDecisionService> decisions;
};

void EpisodeRecognitionControllerTests::init() {
    repository = MemoryHistory();
    progress = RecordingProgress();
    library = FakeLibrary();
    library.items = {entry(21, QStringLiteral("One Piece"), 5), entry(30, QStringLiteral("Frieren"), 0)};
    history = std::make_unique<RecognitionHistoryService>(repository, progress, [] { return Start; });
    reader = std::make_unique<MediaReaderProgressReader>(library);
    decisions = std::make_unique<ProgressDecisionService>(*reader, RecognitionPolicy());
}

void EpisodeRecognitionControllerTests::addUncertain(const QString &key, const int episode) {
    QString error;
    const auto candidate = EpisodeCandidate::create(QStringLiteral("Unknown"), std::nullopt, 1, episode, 0.4).value();
    QVERIFY(history->record(session(key), candidate,
                            {.kind = ProgressDecisionKind::Uncertain, .reason = QStringLiteral("low-confidence")}, error));
}

void EpisodeRecognitionControllerTests::reloadListsNewestFirstWithLibraryTitles() {
    QString error;
    const auto known = EpisodeCandidate::create(QStringLiteral("One Piece"), 21, 1, 6, 1.0).value();
    QVERIFY(history->record(session(QStringLiteral("a")), known, decisions->evaluate(known), error));
    addUncertain(QStringLiteral("b"), 2);
    EpisodeRecognitionController controller(*history, *decisions, library);

    controller.Reload();

    auto &model = controller.model();
    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0), RecognitionListModel::TitleRole).toString(), QStringLiteral("Unknown"));
    QCOMPARE(model.data(model.index(1), RecognitionListModel::TitleRole).toString(), QStringLiteral("One Piece"));
    QCOMPARE(model.data(model.index(1), RecognitionListModel::StatusRole).toString(), QStringLiteral("marked-watched"));
    QVERIFY(!controller.hasMore());
}

void EpisodeRecognitionControllerTests::loadMorePagesWithCursor() {
    for (int index = 1; index <= EpisodeRecognitionController::PageSize + 10; ++index) {
        addUncertain(QStringLiteral("s%1").arg(index), 1);
    }
    EpisodeRecognitionController controller(*history, *decisions, library);

    controller.Reload();
    QCOMPARE(controller.model().rowCount(), EpisodeRecognitionController::PageSize);
    QVERIFY(controller.hasMore());
    controller.LoadMore();

    QCOMPARE(controller.model().rowCount(), EpisodeRecognitionController::PageSize + 10);
    QVERIFY(!controller.hasMore());
}

void EpisodeRecognitionControllerTests::correctionAppliesCorrectedEpisode() {
    addUncertain(QStringLiteral("a"), 2);
    EpisodeRecognitionController controller(*history, *decisions, library);
    controller.Reload();
    const auto recordId = controller.model().data(controller.model().index(0), RecognitionListModel::RecordIdRole).toLongLong();

    QVERIFY2(controller.Correct(recordId, 21, 6), qPrintable(controller.errorMessage()));

    QCOMPARE(progress.applied, QList<int>({6}));
    QCOMPARE(repository.records.first().status(), RecognitionStatus::Corrected);
    QCOMPARE(controller.model().data(controller.model().index(0), RecognitionListModel::TitleRole).toString(),
             QStringLiteral("One Piece"));
    QVERIFY(controller.model().data(controller.model().index(0), RecognitionListModel::CorrectedRole).toBool());
}

void EpisodeRecognitionControllerTests::correctionRejectsInvalidEpisodeOrMedia() {
    addUncertain(QStringLiteral("a"), 2);
    EpisodeRecognitionController controller(*history, *decisions, library);
    controller.Reload();
    const auto recordId = controller.model().data(controller.model().index(0), RecognitionListModel::RecordIdRole).toLongLong();

    QVERIFY(!controller.Correct(recordId, 21, 0));
    QVERIFY(!controller.errorMessage().isEmpty());
    QVERIFY(!controller.Correct(recordId, 0, 3));
    QVERIFY(progress.applied.isEmpty());
}

void EpisodeRecognitionControllerTests::ignoreMarksRecordIgnored() {
    addUncertain(QStringLiteral("a"), 2);
    EpisodeRecognitionController controller(*history, *decisions, library);
    controller.Reload();
    const auto recordId = controller.model().data(controller.model().index(0), RecognitionListModel::RecordIdRole).toLongLong();

    QVERIFY(controller.Ignore(recordId));

    QCOMPARE(repository.records.first().status(), RecognitionStatus::Ignored);
    QCOMPARE(controller.model().data(controller.model().index(0), RecognitionListModel::StatusRole).toString(),
             QStringLiteral("ignored"));
}

void EpisodeRecognitionControllerTests::mediaChoicesListTheLibrary() {
    EpisodeRecognitionController controller(*history, *decisions, library);

    controller.Reload();

    QCOMPARE(controller.mediaChoices().size(), 2);
    QCOMPARE(controller.mediaChoices().at(0).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Frieren"));
    QCOMPARE(controller.mediaChoices().at(1).toMap().value(QStringLiteral("id")).toInt(), 21);
}

void EpisodeRecognitionControllerTests::modelExposesStatusAndCorrectability() {
    addUncertain(QStringLiteral("a"), 2);
    EpisodeRecognitionController controller(*history, *decisions, library);
    controller.Reload();
    auto &model = controller.model();
    const auto index = model.index(0);

    QCOMPARE(model.data(index, RecognitionListModel::StatusRole).toString(), QStringLiteral("uncertain"));
    QCOMPARE(model.data(index, RecognitionListModel::DiagnosticRole).toString(), QStringLiteral("low-confidence"));
    QCOMPARE(model.data(index, RecognitionListModel::EpisodeRole).toInt(), 2);
    QCOMPARE(model.data(index, RecognitionListModel::PluginRole).toString(), QStringLiteral("plex"));
    QCOMPARE(model.data(index, RecognitionListModel::SyncStateRole).toString(), QStringLiteral("not-required"));
    QVERIFY(model.data(index, RecognitionListModel::CorrectableRole).toBool());
    QVERIFY(model.roleNames().values().contains("statusKey"));
}

QTEST_MAIN(EpisodeRecognitionControllerTests)
#include "EpisodeRecognitionControllerTests.moc"
