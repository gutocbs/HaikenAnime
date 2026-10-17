#include <QtTest>

#include "../../src/application/recognition/EpisodeRecognitionService.h"

namespace {
const QDateTime Start(QDate(2026, 10, 17), QTime(20, 0), QTimeZone::UTC);

class ScriptedPlugin final : public IPlaybackPlugin {
public:
    explicit ScriptedPlugin(QString pluginId) : pluginId_(std::move(pluginId)) {}
    [[nodiscard]] QString id() const override { return pluginId_; }
    [[nodiscard]] QString displayName() const override { return pluginId_; }
    [[nodiscard]] PluginCapabilities capabilities() const override { return {}; }
    [[nodiscard]] PluginSettingsSchema settingsSchema() const override { return {.pluginId = pluginId_}; }
    void observe(IPlaybackObserver &value) override { observer = &value; }
    void testConnection(std::function<void(const ConnectionTestResult &)> completion) override { completion({}); }
    void stop() override { ++stops; }

    void play(const QString &key, const EpisodeCandidate &candidate, const int seconds, const QString &user = QStringLiteral("1")) {
        const auto id = PlaybackSessionId::create(pluginId_, key).value();
        observer->onSessionStarted({.id = id, .userId = user, .deviceId = QStringLiteral("tv"), .startedAt = Start},
                                   candidate);
        for (int second = 10; second <= seconds; second += 10) {
            observer->onSessionUpdated(id, {.position = std::chrono::seconds(second), .observedAt = Start.addSecs(second)});
        }
    }
    void close(const QString &key) {
        observer->onSessionStopped(PlaybackSessionId::create(pluginId_, key).value(), StopReason::Closed);
    }

    IPlaybackObserver *observer = nullptr;
    int stops = 0;

private:
    QString pluginId_;
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

class TitleResolver final : public IEpisodeCandidateResolver {
public:
    EpisodeCandidate resolve(const EpisodeCandidate &candidate) override {
        if (candidate.mediaId() || candidate.title() != QStringLiteral("One Piece")) return candidate;
        return EpisodeCandidate::create(candidate.title(), 21, candidate.season(), candidate.episode(),
                                        candidate.confidence()).value();
    }
};

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
        for (const auto &record : records) {
            if (query.recordId && record.id() != *query.recordId) continue;
            if (!query.deduplicationKey.isEmpty() && record.deduplicationKey() != query.deduplicationKey) continue;
            found.append(record);
        }
        return true;
    }
    QList<RecognitionRecord> records;
};

class RecordingProgress final : public IRecognitionProgressWriter {
public:
    bool apply(const Media &, const Media &updated, QString &error) override {
        if (fail) {
            error = QStringLiteral("outbox unavailable");
            return false;
        }
        applied.append(updated.ConsumedChapters);
        return true;
    }
    QList<int> applied;
    bool fail = false;
};

EpisodeCandidate onePiece(const int episode, const std::optional<int> mediaId = std::nullopt,
                          const double confidence = 0.9) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), mediaId, 1, episode, confidence).value();
}

Media libraryEntry(const int consumed) {
    Media media;
    media.Id = 21;
    media.Name = QStringLiteral("One Piece");
    media.ConsumedChapters = consumed;
    media.TotalChapters = 0;
    media.ListStatus = UserListStatus::Current;
    return media;
}
}

class EpisodeRecognitionServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void startObservesEveryRegisteredPlugin();
    void unseenEpisodeAfterThresholdIsMarkedWatchedOnce();
    void alreadyWatchedEpisodeIsRecordedWithoutProgress();
    void lowConfidenceCandidateIsRecordedAsUncertain();
    void duplicateSessionNeverAppliesProgressTwice();
    void closingBeforeThresholdIsRecordedIncomplete();
    void outboxFailureKeepsRecordCorrectable();
    void configuredPolicyChangesThreshold();
    void stopStopsPluginsAndIgnoresLateEvents();
    void catalogResolverMatchesTitleToMedia();
    void catalogResolverKeepsUnknownTitlesUnresolved();

private:
    std::unique_ptr<EpisodeRecognitionService> makeService(RecognitionPolicy policy = RecognitionPolicy());

    PluginRegistry registry;
    ScriptedPlugin *plex = nullptr;
    ScriptedPlugin *mpv = nullptr;
    FakeLibrary library;
    TitleResolver resolver;
    MemoryHistory historyRepository;
    RecordingProgress progress;
    std::unique_ptr<MediaReaderProgressReader> progressReader;
    std::unique_ptr<ProgressDecisionService> decisions;
    std::unique_ptr<RecognitionHistoryService> history;
};

void EpisodeRecognitionServiceTests::init() {
    registry = PluginRegistry();
    auto plexPlugin = std::make_unique<ScriptedPlugin>(QStringLiteral("plex"));
    auto mpvPlugin = std::make_unique<ScriptedPlugin>(QStringLiteral("mpv"));
    plex = plexPlugin.get();
    mpv = mpvPlugin.get();
    registry.registerPlugin(std::move(mpvPlugin));
    registry.registerPlugin(std::move(plexPlugin));
    library = FakeLibrary();
    library.items = {libraryEntry(5)};
    historyRepository = MemoryHistory();
    progress = RecordingProgress();
    progressReader = std::make_unique<MediaReaderProgressReader>(library);
    decisions = std::make_unique<ProgressDecisionService>(*progressReader, RecognitionPolicy());
    history = std::make_unique<RecognitionHistoryService>(historyRepository, progress, [] { return Start; });
}

std::unique_ptr<EpisodeRecognitionService> EpisodeRecognitionServiceTests::makeService(const RecognitionPolicy policy) {
    return std::make_unique<EpisodeRecognitionService>(registry, resolver, *decisions, *history, policy);
}

void EpisodeRecognitionServiceTests::startObservesEveryRegisteredPlugin() {
    auto service = makeService();

    service->start();

    QVERIFY(service->isRunning());
    QCOMPARE(plex->observer, service.get());
    QCOMPARE(mpv->observer, service.get());
}

void EpisodeRecognitionServiceTests::unseenEpisodeAfterThresholdIsMarkedWatchedOnce() {
    auto service = makeService();
    service->start();

    plex->play(QStringLiteral("srv:12"), onePiece(6), 300);
    plex->close(QStringLiteral("srv:12"));

    QCOMPARE(progress.applied, QList<int>({6}));
    QCOMPARE(historyRepository.records.size(), 1);
    QCOMPARE(historyRepository.records.first().status(), RecognitionStatus::MarkedWatched);
    QCOMPARE(historyRepository.records.first().candidate().mediaId(), std::optional<int>(21));
    QCOMPARE(historyRepository.records.first().userId(), QStringLiteral("1"));
}

void EpisodeRecognitionServiceTests::alreadyWatchedEpisodeIsRecordedWithoutProgress() {
    auto service = makeService();
    service->start();

    plex->play(QStringLiteral("srv:12"), onePiece(4), 200);

    QVERIFY(progress.applied.isEmpty());
    QCOMPARE(historyRepository.records.first().status(), RecognitionStatus::AlreadyWatched);
}

void EpisodeRecognitionServiceTests::lowConfidenceCandidateIsRecordedAsUncertain() {
    auto service = makeService();
    service->start();

    plex->play(QStringLiteral("srv:12"), onePiece(6, std::nullopt, 0.3), 200);

    QVERIFY(progress.applied.isEmpty());
    QCOMPARE(historyRepository.records.first().status(), RecognitionStatus::Uncertain);
    QCOMPARE(historyRepository.records.first().diagnostic(), QStringLiteral("low-confidence"));
}

void EpisodeRecognitionServiceTests::duplicateSessionNeverAppliesProgressTwice() {
    auto service = makeService();
    service->start();

    plex->play(QStringLiteral("srv:12"), onePiece(6), 300);
    plex->close(QStringLiteral("srv:12"));
    plex->play(QStringLiteral("srv:12"), onePiece(6), 300);

    QCOMPARE(progress.applied, QList<int>({6}));
    QCOMPARE(historyRepository.records.size(), 1);
}

void EpisodeRecognitionServiceTests::closingBeforeThresholdIsRecordedIncomplete() {
    auto service = makeService();
    service->start();

    mpv->play(QStringLiteral("pipe-1"), onePiece(6, 21, 1.0), 170);
    mpv->close(QStringLiteral("pipe-1"));

    QVERIFY(progress.applied.isEmpty());
    QCOMPARE(historyRepository.records.size(), 1);
    QCOMPARE(historyRepository.records.first().status(), RecognitionStatus::Incomplete);
}

void EpisodeRecognitionServiceTests::outboxFailureKeepsRecordCorrectable() {
    progress.fail = true;
    QStringList diagnostics;
    EpisodeRecognitionService service(registry, resolver, *decisions, *history, RecognitionPolicy(),
                                      [&diagnostics](const QString &message) { diagnostics.append(message); });
    service.start();

    plex->play(QStringLiteral("srv:12"), onePiece(6), 200);

    QCOMPARE(historyRepository.records.first().status(), RecognitionStatus::Uncertain);
    QCOMPARE(historyRepository.records.first().diagnostic(), QStringLiteral("progress-write-failed"));
    QCOMPARE(library.items.first().ConsumedChapters, 5);
}

void EpisodeRecognitionServiceTests::configuredPolicyChangesThreshold() {
    auto service = makeService(RecognitionPolicy::create(600).value());
    service->start();

    plex->play(QStringLiteral("srv:12"), onePiece(6), 300);
    QVERIFY(progress.applied.isEmpty());
    service->setPolicy(RecognitionPolicy::create(60).value());
    plex->play(QStringLiteral("srv:13"), onePiece(7), 60);

    QCOMPARE(progress.applied, QList<int>({7}));
}

void EpisodeRecognitionServiceTests::stopStopsPluginsAndIgnoresLateEvents() {
    auto service = makeService();
    service->start();

    service->stop();
    service->stop();
    plex->play(QStringLiteral("srv:12"), onePiece(6), 300);

    QCOMPARE(plex->stops, 1);
    QCOMPARE(mpv->stops, 1);
    QVERIFY(!service->isRunning());
    QVERIFY(historyRepository.records.isEmpty());
}

void EpisodeRecognitionServiceTests::catalogResolverMatchesTitleToMedia() {
    CatalogEpisodeCandidateResolver catalogResolver(library);

    const auto resolved = catalogResolver.resolve(onePiece(6));

    QCOMPARE(resolved.mediaId(), std::optional<int>(21));
    QCOMPARE(resolved.episode(), 6);
}

void EpisodeRecognitionServiceTests::catalogResolverKeepsUnknownTitlesUnresolved() {
    CatalogEpisodeCandidateResolver catalogResolver(library);
    const auto unknown = EpisodeCandidate::create(QStringLiteral("Unknown Show"), std::nullopt, 1, 2, 0.9).value();

    const auto resolved = catalogResolver.resolve(unknown);

    QVERIFY(!resolved.mediaId().has_value());
    QCOMPARE(resolved.title(), QStringLiteral("Unknown Show"));
}

QTEST_MAIN(EpisodeRecognitionServiceTests)
#include "EpisodeRecognitionServiceTests.moc"
