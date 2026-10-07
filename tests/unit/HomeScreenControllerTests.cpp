#include <QFile>
#include <QRegularExpression>
#include <QtTest>

#include "../../src/presentation/home/HomeScreenController.h"
#include "../../src/application/library/ILocalFileOpener.h"
#include "../../src/application/library/LocalEpisodeReader.h"

class FakeMediaReader final : public IMediaReader {
public:
    QList<Media> result;
    QString failure;

    bool readAll(QList<Media> &media, QString &error) override {
        error = failure;
        media = result;
        return failure.isEmpty();
    }

};

class FakeLocalEpisodeReader final : public ILocalEpisodeReader {
public:
    bool readNextEpisode(int mediaId, int consumedEpisode, LocalEpisode &episode, QString &error) override {
        requestedMediaId = mediaId; requestedConsumedEpisode = consumedEpisode; error = failure;
        episode = result; return failure.isEmpty() && result.episode > 0;
    }
    bool readAvailableEpisodeCount(int, int &count, QString &error) override { count = availableCount; error.clear(); return true; }
    LocalEpisode result;
    QString failure;
    int requestedMediaId = 0;
    int requestedConsumedEpisode = -1;
    int availableCount = 0;
};

class FakeLocalFileOpener final : public ILocalFileOpener {
public:
    bool open(const QString &path, QString &error) override { openedPath = path; error = failure; return failure.isEmpty(); }
    QString failure;
    QString openedPath;
};

class InertCoverDownloader final : public ICoverDownloader {
public:
    quint64 Start(const CoverRequest &request, Completion) override { requests.append(request); return 1; }
    void Cancel(quint64) override {}
    QList<CoverRequest> requests;
};

class MemoryCoverCache final : public ICoverCacheRepository {
public:
    bool ReadAll(QHash<int, CoverCacheEntry> &result, QString &) override { result = entries; return true; }
    bool Upsert(const CoverCacheEntry &entry, QString &) override { entries[entry.mediaId] = entry; return true; }
    bool Remove(int mediaId, QString &) override { entries.remove(mediaId); return true; }
    bool Clear(int &removedEntries, QString &error) override {
        removedEntries = entries.size(); entries.clear(); error.clear(); return true;
    }
    QHash<int, CoverCacheEntry> entries;
};

class ExistingCoverFiles final : public ICoverFileStore {
public:
    bool Exists(const QString &path) const override { return path == QStringLiteral("42.jpg"); }
    bool Publish(int, const QString &, const QString &, const QString &, PublishedCover &, QString &) override { return false; }
    bool Remove(const QString &, QString &) override { return true; }
    bool Clear(int &removedFiles, QString &error) override {
        removedFiles = 0; error.clear(); return true;
    }
    bool RemoveOrphans(const QSet<QString> &, int, int &, QString &) override { return true; }
    QString AbsolutePath(const QString &path) const override { return QStringLiteral("C:/covers/") + path; }
};

class HomeScreenControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void exposesReadyMedia();
    void exposesEmptyState();
    void exposesErrorState();
    void exposesInitializationErrorWithoutRepository();
    void synchronizationCompletionReloadsMedia();
    void exposesPresentationReadyStatus_data();
    void exposesPresentationReadyStatus();
    void exposesSelfDescribingCardValues();
    void exposesPlaceholdersAndConfiguredScoreMaximumOnCards();
    void preparesPreviewCardMetadataForBothStatusPresentations();
    void preparesCompactDetailPreviewMetadataSeparatelyFromCards();
    void preparesCompactDetailPreviewProgressWithZeroTotal();
    void exposesNineItemPreviewAndCompleteFilteredLibrary();
    void reloadReflectsANewLocalPersonalListEntryInPreviewAndFullList();
    void changesMediaTypeUsingStableKeys();
    void exposesBackendDrivenBrowseOptions();
    void restoresConfiguredSortBeforeFirstModelPublication();
    void fallsBackToFirstProvidedSortOption();
    void emitsSortPreferenceOnlyForAcceptedSortChanges();
    void reconcilesActiveCriteriaWhenBackendOptionsChange();
    void appliesListSearchAndSortToBothLibraries();
    void sortsBySeasonYearAndRankInBothDirections();
    void usesResolvedTitleConsistentlyAfterStartupConfiguration();
    void clearsBrowseCriteriaWithoutChangingMediaType();
    void selectsMediaAndExposesItsDetails();
    void presentsCompleteExtendedMediaDetails();
    void presentsMissingExtendedMediaDetailsDeterministically();
    void keepsLongSynopsisAvailableForDetailsPresentation();
    void presentsNextAiringEpisodeAndTimestamp();
    void exposesControllerApprovedDeduplicatedMediaLinks();
    void fullMediaDetailsPanelProvidesSafeInteractiveDetails();
    void exposesConfigurableEditingOptions();
    void selectedCoverFallsBackWhenCachedFileIsMissing();
    void updatesOnlyOneCoverRowAndPreservesOldCoverOnFailure();
    void coverQualityChangesFutureRequestsWithoutClearingDisplayedCover();
    void exposesAndOpensNextLocalEpisodeForSelectedAnime();
    void reportsLocalEpisodeLookupAndOpenFailures();
    void refreshesNextLocalEpisodeAfterRecognitionCompletes();

private:
    static QString qmlSource(const QString &name);
};

QString HomeScreenControllerTests::qmlSource(const QString &name) {
    QFile file(QStringLiteral(HAIKENANIME_TEST_SOURCE_DIR "/resources/qml/") + name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll());
}

void HomeScreenControllerTests::reloadReflectsANewLocalPersonalListEntryInPreviewAndFullList() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);
    controller.reload();
    QCOMPARE(controller.mediaModel()->rowCount(), 0);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 0);

    Media added;
    added.Id = 77;
    added.Name = QStringLiteral("Seasonal addition");
    added.Type = MediaType::Anime;
    added.ListStatus = UserListStatus::Planning;
    reader.result.append(added);
    controller.reload();

    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(0, 0),
                                                HomeMediaModel::IdRole).toInt(), added.Id);
}

void HomeScreenControllerTests::coverQualityChangesFutureRequestsWithoutClearingDisplayedCover() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.CoverUrl = QStringLiteral("https://img/original.jpg");
    media.CoverMediumUrl = QStringLiteral("https://img/medium.jpg");
    media.CoverLargeUrl = QStringLiteral("https://img/large.jpg");
    reader.result.append(media);
    InertCoverDownloader downloader;
    MemoryCoverCache cache;
    ExistingCoverFiles files;
    CoverSettings settings;
    CoverDownloadCoordinator covers(downloader, cache, files, settings);
    HomeScreenController controller(&reader, &covers, CoverQuality::Medium);
    controller.reload();
    controller.RequestCoverWindow(QStringLiteral("preview"), 0, 0, 0);
    QCOMPARE(downloader.requests.constLast().remoteUrl, QUrl(media.CoverMediumUrl));

    controller.ConfigureCoverQuality(CoverQuality::Large);
    controller.RequestCoverWindow(QStringLiteral("preview"), 0, 0, 0);
    QCOMPARE(downloader.requests.constLast().remoteUrl, QUrl(media.CoverLargeUrl));
}

void HomeScreenControllerTests::exposesReadyMedia() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.TotalChapters = 28;
    media.ConsumedChapters = 12;
    media.PersonalScore = 9;
    media.ListStatus = UserListStatus::Current;
    media.AlternativeNames = {QStringLiteral("Frieren at the Funeral"), QStringLiteral("Frieren")};
    media.Type = MediaType::Anime;
    reader.result.append(media);

    HomeScreenController controller(reader);
    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("ready"));
    QCOMPARE(controller.statusMessage(), QStringLiteral("Dados locais carregados."));
    QCOMPARE(controller.mediaCount(), 1);
    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::TitleRole).toString(),
             QStringLiteral("Frieren"));
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ProgressRole).toString(),
             QStringLiteral("Progresso 12/28"));
}

void HomeScreenControllerTests::exposesEmptyState() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("empty"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesErrorState() {
    FakeMediaReader reader;
    reader.failure = QStringLiteral("Database unavailable");
    HomeScreenController controller(reader);

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Database unavailable"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesInitializationErrorWithoutRepository() {
    HomeScreenController controller(nullptr, QStringLiteral("Database initialization failed"));

    controller.reload();

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Database initialization failed"));
    QCOMPARE(controller.statusMessage(), QStringLiteral("Não foi possível carregar os dados locais."));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::synchronizationCompletionReloadsMedia() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    reader.result.append(media);
    HomeScreenController controller(reader);

    controller.notifySynchronizationCompleted();

    QCOMPARE(controller.state(), QStringLiteral("ready"));
    QCOMPARE(controller.mediaCount(), 1);
    QCOMPARE(controller.statusMessage(), QStringLiteral("Dados locais carregados."));
}

void HomeScreenControllerTests::exposesPresentationReadyStatus_data() {
    QTest::addColumn<int>("status");
    QTest::addColumn<QString>("expectedLabel");

    QTest::newRow("unknown") << static_cast<int>(MediaStatus::Unknown)
                              << QStringLiteral("Desconhecido");
    QTest::newRow("not released") << static_cast<int>(MediaStatus::NotReleased)
                                   << QStringLiteral("Ainda não lançado");
    QTest::newRow("releasing") << static_cast<int>(MediaStatus::Releasing)
                                << QStringLiteral("Em lançamento");
    QTest::newRow("released") << static_cast<int>(MediaStatus::Released)
                               << QStringLiteral("Concluído");
}

void HomeScreenControllerTests::exposesPresentationReadyStatus() {
    QFETCH(int, status);
    QFETCH(QString, expectedLabel);
    const auto expectedCardLabel = QStringLiteral("Exibição: %1").arg(expectedLabel);
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Status = static_cast<MediaStatus>(status);
    media.Type = MediaType::Anime;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.ConfigureCardStatusPresentation(CardStatusPresentation::MediaReleaseStatus);

    controller.reload();

    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::StatusLabelRole).toString(),
             expectedCardLabel);
}

void HomeScreenControllerTests::exposesNineItemPreviewAndCompleteFilteredLibrary() {
    FakeMediaReader reader;
    for (int id = 1; id <= 12; ++id) {
        Media item;
        item.Id = id;
        item.Name = QStringLiteral("Media %1").arg(id);
        item.Type = MediaType::Anime;
        reader.result.append(item);
    }
    Media manga;
    manga.Id = 13;
    manga.Name = QStringLiteral("Manga");
    manga.Type = MediaType::Manga;
    reader.result.append(manga);

    HomeScreenController controller(reader);
    controller.reload();

    QCOMPARE(controller.mediaModel()->rowCount(), 9);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 12);
    QCOMPARE(controller.filteredMediaCount(), 12);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(11, 0),
                                                HomeMediaModel::IdRole).toInt(), 9);
}

void HomeScreenControllerTests::changesMediaTypeUsingStableKeys() {
    FakeMediaReader reader;
    Media anime;
    anime.Id = 1;
    anime.Name = QStringLiteral("Anime");
    anime.Type = MediaType::Anime;
    Media manga;
    manga.Id = 2;
    manga.Name = QStringLiteral("Manga");
    manga.Type = MediaType::Manga;
    Media novel;
    novel.Id = 3;
    novel.Name = QStringLiteral("Novel");
    novel.Type = MediaType::Novel;
    reader.result = {anime, manga, novel};

    HomeScreenController controller(reader);
    controller.reload();
    controller.SetMediaType(QStringLiteral("manga"));

    QCOMPARE(controller.activeMediaType(), QStringLiteral("manga"));
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                            HomeMediaModel::IdRole).toInt(), 2);
    controller.SetMediaType(QStringLiteral("invalid"));
    QCOMPARE(controller.activeMediaType(), QStringLiteral("manga"));
}

void HomeScreenControllerTests::exposesBackendDrivenBrowseOptions() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);

    controller.ConfigureBrowseOptions(
        QVariantList{
            QVariantMap{{QStringLiteral("key"), QStringLiteral("anime")},
                        {QStringLiteral("label"), QStringLiteral("Animation")}}},
        QVariantList{
            QVariantMap{{QStringLiteral("key"), QStringLiteral("all")},
                        {QStringLiteral("label"), QStringLiteral("Everything")}},
            QVariantMap{{QStringLiteral("key"), QStringLiteral("current")},
                        {QStringLiteral("label"), QStringLiteral("In progress")}}},
        QVariantList{
            QVariantMap{{QStringLiteral("key"), QStringLiteral("title_asc")},
                        {QStringLiteral("label"), QStringLiteral("Name")}}});

    const auto listOptions = controller.availableListOptions();
    const auto sortOptions = controller.availableSortOptions();
    const auto mediaTypeOptions = controller.availableMediaTypeOptions();

    QCOMPARE(mediaTypeOptions.size(), 1);
    QCOMPARE(mediaTypeOptions.first().toMap().value(QStringLiteral("label")).toString(),
             QStringLiteral("Animation"));
    QCOMPARE(listOptions.size(), 2);
    QCOMPARE(listOptions.first().toMap().value(QStringLiteral("key")).toString(), QStringLiteral("all"));
    QCOMPARE(listOptions.at(1).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("current"));
    QCOMPARE(listOptions.at(1).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("In progress"));
    QCOMPARE(sortOptions.size(), 1);
    QCOMPARE(sortOptions.first().toMap().value(QStringLiteral("key")).toString(), QStringLiteral("title_asc"));
    QCOMPARE(controller.activeListFilter(), QStringLiteral("all"));
    QCOMPARE(controller.activeSort(), QStringLiteral("title_asc"));
}

void HomeScreenControllerTests::exposesSelfDescribingCardValues() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Completed;
    media.Status = MediaStatus::Released;
    media.ConsumedChapters = 12;
    media.TotalChapters = 24;
    media.PersonalScore = 9;
    reader.result.append(media);
    HomeScreenController controller(reader);

    controller.reload();

    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::StatusLabelRole).toString(),
             QStringLiteral("Minha lista: Concluído"));
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ProgressRole).toString(),
             QStringLiteral("Progresso 12/24"));
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ScoreRole).toString(),
             QStringLiteral("Nota 9/10"));
}

void HomeScreenControllerTests::exposesPlaceholdersAndConfiguredScoreMaximumOnCards() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.TotalChapters = 0;
    media.ConsumedChapters = 0;
    media.PersonalScore = 0;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.ConfigureScoreScale(0.0, 100.0, 5.0);

    controller.reload();

    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ProgressRole).toString(),
             QStringLiteral("Progresso —"));
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ScoreRole).toString(),
             QStringLiteral("Nota —"));
    reader.result.first().PersonalScore = 85;
    controller.reload();
    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                            HomeMediaModel::ScoreRole).toString(),
             QStringLiteral("Nota 85/100"));
}

void HomeScreenControllerTests::preparesPreviewCardMetadataForBothStatusPresentations() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.Status = MediaStatus::Released;
    media.TotalChapters = 24;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.ConfigureScoreScale(0.0, 100.0, 5.0);
    controller.reload();
    controller.SelectMedia(42);

    const auto personal = controller.PreviewCardMetadata(12, QStringLiteral("completed"), 85);
    QCOMPARE(personal.value(QStringLiteral("status")).toString(),
             QStringLiteral("Minha lista: Concluído"));
    QCOMPARE(personal.value(QStringLiteral("progress")).toString(),
             QStringLiteral("Progresso 12/24"));
    QCOMPARE(personal.value(QStringLiteral("score")).toString(), QStringLiteral("Nota 85/100"));

    controller.ConfigureCardStatusPresentation(CardStatusPresentation::MediaReleaseStatus);
    const auto release = controller.PreviewCardMetadata(12, QStringLiteral("completed"), 85);
    QCOMPARE(release.value(QStringLiteral("status")).toString(), QStringLiteral("Exibição: Concluído"));
    QCOMPARE(release.value(QStringLiteral("progress")).toString(), QStringLiteral("Progresso 12/24"));
    QCOMPARE(release.value(QStringLiteral("score")).toString(), QStringLiteral("Nota 85/100"));
}

void HomeScreenControllerTests::preparesCompactDetailPreviewMetadataSeparatelyFromCards() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.Status = MediaStatus::Released;
    media.TotalChapters = 24;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.ConfigureScoreScale(0.0, 100.0, 5.0);
    controller.reload();
    controller.SelectMedia(42);

    const auto detail = controller.PreviewCompactDetailMetadata(12, QStringLiteral("completed"), 85);
    QCOMPARE(detail.value(QStringLiteral("status")).toString(), QStringLiteral("Concluídas"));
    QCOMPARE(detail.value(QStringLiteral("progress")).toString(), QStringLiteral("12/24"));
    QCOMPARE(detail.value(QStringLiteral("score")).toString(), QStringLiteral("85"));

    controller.ConfigureCardStatusPresentation(CardStatusPresentation::MediaReleaseStatus);
    const auto detailWithReleaseCards = controller.PreviewCompactDetailMetadata(
        12, QStringLiteral("completed"), 85);
    QCOMPARE(detailWithReleaseCards, detail);
}

void HomeScreenControllerTests::preparesCompactDetailPreviewProgressWithZeroTotal() {
    FakeMediaReader reader;
    Media media;
    media.Id = 43;
    media.Name = QStringLiteral("Unknown total");
    media.Type = MediaType::Anime;
    media.TotalChapters = 0;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SelectMedia(43);

    const auto detail = controller.PreviewCompactDetailMetadata(12, QStringLiteral("current"), 0.0);
    QCOMPARE(detail.value(QStringLiteral("progress")).toString(), QStringLiteral("12/0"));
}

void HomeScreenControllerTests::restoresConfiguredSortBeforeFirstModelPublication() {
    FakeMediaReader reader;
    Media first; first.Id = 1; first.Name = QStringLiteral("Alpha"); first.Type = MediaType::Anime;
    Media second; second.Id = 2; second.Name = QStringLiteral("Zulu"); second.Type = MediaType::Anime;
    reader.result = {first, second};
    HomeScreenController controller(reader);

    controller.ConfigureInitialSort(QStringLiteral("title_desc"));
    controller.reload();

    QCOMPARE(controller.activeSort(), QStringLiteral("title_desc"));
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(0, 0),
                                                HomeMediaModel::IdRole).toInt(), 2);
}

void HomeScreenControllerTests::fallsBackToFirstProvidedSortOption() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);
    controller.ConfigureBrowseOptions(
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("anime")},
                                 {QStringLiteral("label"), QStringLiteral("Anime")}}},
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("all")},
                                 {QStringLiteral("label"), QStringLiteral("All")}}},
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("progress")},
                                 {QStringLiteral("label"), QStringLiteral("Progress")}},
                     QVariantMap{{QStringLiteral("key"), QStringLiteral("title_asc")},
                                 {QStringLiteral("label"), QStringLiteral("Title")}}});

    controller.ConfigureInitialSort(QStringLiteral("removed_sort"));

    QCOMPARE(controller.activeSort(), QStringLiteral("progress"));
}

void HomeScreenControllerTests::emitsSortPreferenceOnlyForAcceptedSortChanges() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);
    QSignalSpy changed(&controller, &HomeScreenController::sortPreferenceChanged);

    controller.SetSort(QStringLiteral("unsupported"));
    controller.SetMediaType(QStringLiteral("manga"));
    controller.SetListFilter(QStringLiteral("current"));
    controller.SetSearchQuery(QStringLiteral("frieren"));
    QCOMPARE(changed.count(), 0);

    controller.SetSort(QStringLiteral("title_desc"));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.takeFirst().at(0).toString(), QStringLiteral("title_desc"));

    controller.ClearBrowseCriteria();
    QCOMPARE(changed.count(), 1);
    QCOMPARE(changed.takeFirst().at(0).toString(), QStringLiteral("title_asc"));
}

void HomeScreenControllerTests::reconcilesActiveCriteriaWhenBackendOptionsChange() {
    FakeMediaReader reader;
    Media manga;
    manga.Id = 7;
    manga.Name = QStringLiteral("Yotsuba");
    manga.Type = MediaType::Manga;
    manga.ListStatus = UserListStatus::Planning;
    reader.result.append(manga);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SetListFilter(QStringLiteral("completed"));
    controller.SetSort(QStringLiteral("title_desc"));

    controller.ConfigureBrowseOptions(
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("manga")},
                                 {QStringLiteral("label"), QStringLiteral("Manga")}}},
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("planning")},
                                 {QStringLiteral("label"), QStringLiteral("Planning")}}},
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("title_asc")},
                                 {QStringLiteral("label"), QStringLiteral("Name")}}});

    QCOMPARE(controller.activeMediaType(), QStringLiteral("manga"));
    QCOMPARE(controller.activeListFilter(), QStringLiteral("planning"));
    QCOMPARE(controller.activeSort(), QStringLiteral("title_asc"));
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    QVERIFY(!controller.browseCriteriaActive());

    controller.ClearBrowseCriteria();
    QCOMPARE(controller.activeListFilter(), QStringLiteral("planning"));
    QCOMPARE(controller.activeSort(), QStringLiteral("title_asc"));
    QVERIFY(!controller.browseCriteriaActive());
}

void HomeScreenControllerTests::appliesListSearchAndSortToBothLibraries() {
    FakeMediaReader reader;
    for (int id = 1; id <= 12; ++id) {
        Media item;
        item.Id = id;
        item.Name = QStringLiteral("Title %1").arg(id, 2, 10, QLatin1Char('0'));
        item.Type = MediaType::Anime;
        item.ListStatus = id <= 10 ? UserListStatus::Current : UserListStatus::Completed;
        item.PersonalScore = id;
        item.ConsumedChapters = id;
        reader.result.append(item);
    }

    HomeScreenController controller(reader);
    controller.reload();
    controller.SetListFilter(QStringLiteral("current"));
    controller.SetSort(QStringLiteral("personal_score"));
    controller.SetSearchQuery(QStringLiteral("title"));

    QCOMPARE(controller.filteredMediaCount(), 10);
    QCOMPARE(controller.mediaModel()->rowCount(), 9);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 10);
    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                            HomeMediaModel::IdRole).toInt(), 10);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(9, 0),
                                                HomeMediaModel::IdRole).toInt(), 1);
}

void HomeScreenControllerTests::sortsBySeasonYearAndRankInBothDirections() {
    FakeMediaReader reader;
    Media winter;
    winter.Id = 1;
    winter.Name = QStringLiteral("Winter");
    winter.Type = MediaType::Anime;
    winter.Season = QStringLiteral("WINTER");
    winter.SeasonYear = 2024;
    Media spring;
    spring.Id = 2;
    spring.Name = QStringLiteral("Spring");
    spring.Type = MediaType::Anime;
    spring.Season = QStringLiteral("SPRING");
    spring.SeasonYear = 2024;
    Media olderFall;
    olderFall.Id = 3;
    olderFall.Name = QStringLiteral("Older fall");
    olderFall.Type = MediaType::Anime;
    olderFall.Season = QStringLiteral("FALL");
    olderFall.SeasonYear = 2023;
    Media undated;
    undated.Id = 4;
    undated.Name = QStringLiteral("Undated");
    undated.Type = MediaType::Anime;
    reader.result = {spring, undated, olderFall, winter};

    HomeScreenController controller(reader);
    controller.ConfigureInitialSort(QStringLiteral("season_asc"));
    controller.reload();

    QCOMPARE(controller.activeSort(), QStringLiteral("season_asc"));
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(0, 0),
                                                HomeMediaModel::IdRole).toInt(), 4);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(1, 0),
                                                HomeMediaModel::IdRole).toInt(), 3);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(2, 0),
                                                HomeMediaModel::IdRole).toInt(), 1);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(3, 0),
                                                HomeMediaModel::IdRole).toInt(), 2);

    controller.SetSort(QStringLiteral("season_desc"));
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(0, 0),
                                                HomeMediaModel::IdRole).toInt(), 2);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(1, 0),
                                                HomeMediaModel::IdRole).toInt(), 1);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(2, 0),
                                                HomeMediaModel::IdRole).toInt(), 3);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(3, 0),
                                                HomeMediaModel::IdRole).toInt(), 4);
}

void HomeScreenControllerTests::usesResolvedTitleConsistentlyAfterStartupConfiguration() {
    FakeMediaReader reader;
    Media first;
    first.Id = 42;
    first.Name = QStringLiteral("A Romaji");
    first.EnglishName = QStringLiteral("Zulu English");
    first.OriginalName = QStringLiteral("ネイティブ A");
    first.Type = MediaType::Anime;
    Media second;
    second.Id = 43;
    second.Name = QStringLiteral("Z Romaji");
    second.EnglishName = QStringLiteral("Alpha English");
    second.OriginalName = QStringLiteral("ネイティブ Z");
    second.Type = MediaType::Anime;
    reader.result = {first, second};

    HomeScreenController controller(reader);
    controller.ConfigurePreferredTitle(QStringLiteral("english"));
    controller.reload();

    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(0, 0),
                                                HomeMediaModel::TitleRole).toString(),
             QStringLiteral("Alpha English"));
    controller.SelectMedia(43);
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Alpha English"));
    controller.SetSearchQuery(QStringLiteral("alpha english"));
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    QCOMPARE(controller.fullMediaModel()->data(controller.fullMediaModel()->index(0, 0),
                                                HomeMediaModel::IdRole).toInt(), 43);
    controller.SetSearchQuery(QStringLiteral("Z Romaji"));
    QCOMPARE(controller.fullMediaModel()->rowCount(), 0);
}

void HomeScreenControllerTests::clearsBrowseCriteriaWithoutChangingMediaType() {
    FakeMediaReader reader;
    Media manga;
    manga.Id = 7;
    manga.Name = QStringLiteral("Yotsuba");
    manga.Type = MediaType::Manga;
    manga.ListStatus = UserListStatus::Planning;
    reader.result.append(manga);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SetMediaType(QStringLiteral("manga"));
    controller.SetListFilter(QStringLiteral("completed"));
    controller.SetSort(QStringLiteral("title_desc"));
    controller.SetSearchQuery(QStringLiteral("missing"));

    controller.ClearBrowseCriteria();

    QCOMPARE(controller.activeMediaType(), QStringLiteral("manga"));
    QCOMPARE(controller.activeListFilter(), QStringLiteral("all"));
    QCOMPARE(controller.activeSort(), QStringLiteral("title_asc"));
    QVERIFY(controller.searchQuery().isEmpty());
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
}

void HomeScreenControllerTests::selectsMediaAndExposesItsDetails() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Synopsis = QStringLiteral("An elf retraces a decade-long journey.");
    media.Type = MediaType::Anime;
    media.Status = MediaStatus::Released;
    media.TotalChapters = 28;
    media.ConsumedChapters = 12;
    media.AverageScore = 88;
    media.PersonalScore = 9;
    media.ListStatus = UserListStatus::Current;
    media.AlternativeNames = {QStringLiteral("Frieren at the Funeral"), QStringLiteral("Frieren")};
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();

    controller.SelectMedia(42);

    QVERIFY(controller.hasSelection());
    QCOMPARE(controller.selectedMediaId(), 42);
    QCOMPARE(controller.selectedTitle(), QStringLiteral("Frieren"));
    QCOMPARE(controller.selectedSynopsis(), media.Synopsis);
    QCOMPARE(controller.selectedTypeLabel(), QStringLiteral("Anime"));
    QCOMPARE(controller.selectedStatusLabel(), QStringLiteral("Concluído"));
    QCOMPARE(controller.selectedProgress(), QStringLiteral("12/28"));
    QCOMPARE(controller.selectedScore(), QStringLiteral("9"));
    QCOMPARE(controller.selectedProgressValue(), 12);
    QCOMPARE(controller.selectedProgressMaximum(), 28);
    QCOMPARE(controller.selectedScoreValue(), 9.0);
    QCOMPARE(controller.selectedListStatusKey(), QStringLiteral("current"));
    QCOMPARE(controller.selectedAlternativeNames(), media.AlternativeNames);

    controller.SetMediaType(QStringLiteral("manga"));
    QVERIFY(!controller.hasSelection());
    QCOMPARE(controller.filteredMediaCount(), 0);
    QCOMPARE(controller.state(), QStringLiteral("ready"));
}

void HomeScreenControllerTests::presentsCompleteExtendedMediaDetails() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Synopsis = QStringLiteral("A long-lived elf begins a new journey.");
    media.Type = MediaType::Anime;
    media.Status = MediaStatus::Releasing;
    media.TotalChapters = 28;
    media.ConsumedChapters = 12;
    media.AverageScore = 88;
    media.PersonalScore = 9;
    media.Season = QStringLiteral("FALL");
    media.SeasonYear = 2026;
    media.NextAiringEpisode = 5;
    media.NextAiringAt = 1790834400;
    media.AniListUrl = QStringLiteral("https://anilist.co/anime/154587");
    media.ExternalLinks = {{QStringLiteral("Crunchyroll"), QStringLiteral("https://www.crunchyroll.com/frieren")}};
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SelectMedia(media.Id);

    QCOMPARE(controller.selectedSeasonLabel(), QStringLiteral("Outono de 2026"));
    QCOMPARE(controller.selectedNextAiringLabel(), QStringLiteral("Episódio 5 · 01/10/2026 06:00 UTC"));
    const QVariantList links = controller.selectedMediaLinks();
    QCOMPARE(links.size(), 2);
    QCOMPARE(links.at(0).toMap().value(QStringLiteral("site")).toString(), QStringLiteral("AniList"));
    QCOMPARE(links.at(0).toMap().value(QStringLiteral("url")).toString(), media.AniListUrl);
    QCOMPARE(links.at(1).toMap().value(QStringLiteral("site")).toString(), QStringLiteral("Crunchyroll"));
}

void HomeScreenControllerTests::presentsMissingExtendedMediaDetailsDeterministically() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Unknown media");
    media.Type = MediaType::Anime;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SelectMedia(media.Id);

    QCOMPARE(controller.selectedSeasonLabel(), QString());
    QCOMPARE(controller.selectedNextAiringLabel(), QString());
    QVERIFY(controller.selectedMediaLinks().isEmpty());
}

void HomeScreenControllerTests::keepsLongSynopsisAvailableForDetailsPresentation() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Long synopsis");
    media.Type = MediaType::Anime;
    media.Synopsis = QString(1200, QLatin1Char('x'));
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SelectMedia(media.Id);

    QCOMPARE(controller.selectedSynopsis().size(), 1200);
    QCOMPARE(controller.selectedSynopsis(), media.Synopsis);
}

void HomeScreenControllerTests::presentsNextAiringEpisodeAndTimestamp() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Airing media");
    media.Type = MediaType::Anime;
    media.NextAiringEpisode = 7;
    media.NextAiringAt = 0;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SelectMedia(media.Id);

    QCOMPARE(controller.selectedNextAiringLabel(), QStringLiteral("Episódio 7 · 01/01/1970 00:00 UTC"));
}

void HomeScreenControllerTests::exposesControllerApprovedDeduplicatedMediaLinks() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Linked media");
    media.Type = MediaType::Anime;
    media.AniListUrl = QStringLiteral("https://anilist.co/anime/42");
    media.ExternalLinks = {
        {QStringLiteral("anilist"), QStringLiteral("https://anilist.co/anime/42")},
        {QStringLiteral("Crunchyroll"), QStringLiteral("https://www.crunchyroll.com/series/42")},
        {QStringLiteral("crunchyroll"), QStringLiteral("https://www.crunchyroll.com/watch/42/episode-2")}
    };
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.reload();
    controller.SelectMedia(media.Id);

    const QVariantList links = controller.selectedMediaLinks();
    QCOMPARE(links.size(), 2);
    QCOMPARE(links.at(0).toMap().value(QStringLiteral("site")).toString(), QStringLiteral("AniList"));
    QCOMPARE(links.at(0).toMap().value(QStringLiteral("url")).toString(), media.AniListUrl);
    QCOMPARE(links.at(1).toMap().value(QStringLiteral("site")).toString(), QStringLiteral("Crunchyroll"));
    QCOMPARE(links.at(1).toMap().value(QStringLiteral("url")).toString(), media.ExternalLinks.at(1).Url);
}

void HomeScreenControllerTests::fullMediaDetailsPanelProvidesSafeInteractiveDetails() {
    const QString homeSource = qmlSource(QStringLiteral("Home.qml"));
    const QString panelSource = qmlSource(QStringLiteral("MediaDetailsPanel.qml"));
    QVERIFY(!homeSource.isEmpty());
    QVERIFY2(!panelSource.isEmpty(), "Full media details must use a dedicated QML panel.");

    QVERIFY(homeSource.contains(QStringLiteral("text: qsTr(\"Ver detalhes\")")));
    QVERIFY(homeSource.contains(QStringLiteral("mediaDetailsPanel.openForItem(detailsButton)")));
    QVERIFY(panelSource.contains(QStringLiteral("x: 0")));
    QVERIFY(panelSource.contains(QStringLiteral("modal: true")));
    QVERIFY(panelSource.contains(QStringLiteral("focus: true")));
    QVERIFY(panelSource.contains(QStringLiteral("Popup.CloseOnEscape | Popup.CloseOnPressOutside")));
    QVERIFY(panelSource.contains(QStringLiteral("onOpened: closeButton.forceActiveFocus()")));
    QVERIFY(panelSource.contains(QStringLiteral("returnFocusItem.forceActiveFocus()")));
    QVERIFY(panelSource.contains(QRegularExpression(
        QStringLiteral(R"(Keys\.onTabPressed\s*:\s*function\(event\)\s*\{[^}]*closeButton\.forceActiveFocus\(\)[^}]*event\.accepted\s*=\s*true)"))));
    QVERIFY(panelSource.contains(QRegularExpression(
        QStringLiteral(R"(Keys\.onBacktabPressed\s*:\s*function\(event\)\s*\{[^}]*closeButton\.forceActiveFocus\(\)[^}]*event\.accepted\s*=\s*true)"))));

    QVERIFY(panelSource.contains(QStringLiteral("ScrollView")));
    QVERIFY(panelSource.contains(QStringLiteral("text: controller.selectedSynopsis")));
    QVERIFY(panelSource.contains(QStringLiteral("readOnly: true")));
    QVERIFY(panelSource.contains(QStringLiteral("selectByMouse: true")));
    QVERIFY(panelSource.contains(QStringLiteral("id: fullSynopsisText")));
    QVERIFY(panelSource.contains(QStringLiteral("model: controller.selectedMediaLinks")));
    QVERIFY(panelSource.contains(QStringLiteral("id: externalLinkButton")));
    QVERIFY(panelSource.contains(QStringLiteral("Qt.openUrlExternally(modelData.url)")));
    QVERIFY(homeSource.contains(QStringLiteral("id: watchNextButton")));
    QVERIFY(homeSource.contains(QStringLiteral("text: qsTr(\"Assistir\")")));
    QVERIFY(homeSource.contains(QStringLiteral("enabled: controller.canWatch")));
    QVERIFY(homeSource.contains(QStringLiteral("controller.WatchNext()")));
    QVERIFY(homeSource.contains(QStringLiteral("controller.localLibraryStatusMessage")));
    QVERIFY(homeSource.contains(QStringLiteral("BIBLIOTECA LOCAL")));
    QVERIFY(!panelSource.contains(QStringLiteral("watchNextButton")));
    QVERIFY(!panelSource.contains(QStringLiteral("controller.WatchNext()")));
    QVERIFY(!panelSource.contains(QStringLiteral("nextLocalEpisodePath")));
    QVERIFY(!panelSource.contains(QStringLiteral("read-next-local-episode.sql")));
    QCOMPARE(panelSource.count(QStringLiteral("panel.close()")), 1);

    const qsizetype synopsisStart = panelSource.indexOf(QStringLiteral("id: fullSynopsisText"));
    const qsizetype linkStart = panelSource.indexOf(QStringLiteral("id: externalLinkButton"));
    QVERIFY(synopsisStart >= 0);
    QVERIFY(linkStart > synopsisStart);
    const QString synopsisSection = panelSource.mid(synopsisStart, linkStart - synopsisStart);
    const QString linkSection = panelSource.mid(linkStart);
    QVERIFY(!synopsisSection.contains(QStringLiteral("panel.close()")));
    QVERIFY(!linkSection.contains(QStringLiteral("panel.close()")));
}

void HomeScreenControllerTests::exposesConfigurableEditingOptions() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);

    QCOMPARE(controller.scoreMinimum(), 0.0);
    QCOMPARE(controller.scoreMaximum(), 10.0);
    QCOMPARE(controller.scoreStep(), 1.0);

    controller.ConfigureScoreScale(0.0, 100.0, 5.0);

    QCOMPARE(controller.scoreMinimum(), 0.0);
    QCOMPARE(controller.scoreMaximum(), 100.0);
    QCOMPARE(controller.scoreStep(), 5.0);
}

void HomeScreenControllerTests::selectedCoverFallsBackWhenCachedFileIsMissing() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.CoverUrl = QStringLiteral("https://example/42.jpg");
    reader.result.append(media);

    InertCoverDownloader downloader;
    MemoryCoverCache cache;
    cache.entries[42] = {42, media.CoverUrl, CoverQuality::Medium, QStringLiteral("42.jpg"),
                         QStringLiteral("image/jpeg"), 100};
    ExistingCoverFiles files;
    CoverSettings settings;
    CoverDownloadCoordinator covers(downloader, cache, files, settings);
    HomeScreenController controller(&reader, &covers, CoverQuality::Medium);
    controller.reload();
    controller.RequestCoverWindow(QStringLiteral("preview"), 0, 0, 0);
    controller.SelectMedia(42);
    QCOMPARE(controller.selectedCoverSource(), QStringLiteral("file:///C:/covers/42.jpg"));

    controller.ReportCoverLoadFailure(42);

    QCOMPARE(controller.selectedCoverSource(),
             QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"));
}

void HomeScreenControllerTests::updatesOnlyOneCoverRowAndPreservesOldCoverOnFailure() {
    HomeMediaModel model;
    Media first; first.Id = 42; first.CoverUrl = QStringLiteral("https://example/42.jpg");
    Media second; second.Id = 43;
    model.setMedia({first, second});
    const auto index = model.index(0, 0);
    QCOMPARE(model.data(index, HomeMediaModel::CoverSourceRole).toString(),
             QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"));
    QCOMPARE(model.data(index, HomeMediaModel::RemoteCoverUrlRole).toString(), first.CoverUrl);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
    QVERIFY(model.UpdateCover(42, QStringLiteral("file:///covers/42.jpg"), CoverState::Available));
    QCOMPARE(changed.count(), 1); QCOMPARE(reset.count(), 0);
    model.UpdateCover(42, {}, CoverState::Failed);
    QCOMPARE(model.data(index, HomeMediaModel::CoverSourceRole).toString(), QStringLiteral("file:///covers/42.jpg"));
    model.UpdateCover(42, {}, CoverState::Missing);
    QCOMPARE(model.data(index, HomeMediaModel::CoverSourceRole).toString(),
             QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"));
}

void HomeScreenControllerTests::exposesAndOpensNextLocalEpisodeForSelectedAnime() {
    FakeMediaReader reader;
    Media media; media.Id = 42; media.Name = QStringLiteral("Example"); media.Type = MediaType::Anime;
    media.ConsumedChapters = 2; reader.result = {media};
    FakeLocalEpisodeReader episodes;
    episodes.result = {42, 3, QStringLiteral("Q:/Animes/Example/03.mkv")};
    FakeLocalFileOpener opener;
    HomeScreenController controller(reader);
    controller.SetLocalEpisodeServices(&episodes, &opener);
    controller.reload(); controller.SelectMedia(42);
    QVERIFY(controller.canWatch());
    QCOMPARE(controller.nextLocalEpisode(), 3);
    QCOMPARE(episodes.requestedMediaId, 42);
    QCOMPARE(episodes.requestedConsumedEpisode, 2);
    controller.WatchNext();
    QCOMPARE(opener.openedPath, QStringLiteral("Q:/Animes/Example/03.mkv"));
}

void HomeScreenControllerTests::reportsLocalEpisodeLookupAndOpenFailures() {
    FakeMediaReader reader;
    Media media; media.Id = 42; media.Name = QStringLiteral("Example"); media.Type = MediaType::Anime;
    reader.result = {media};
    FakeLocalEpisodeReader episodes; episodes.failure = QStringLiteral("Lookup failed");
    FakeLocalFileOpener opener;
    HomeScreenController controller(reader);
    controller.SetLocalEpisodeServices(&episodes, &opener);
    controller.reload(); controller.SelectMedia(42);
    QVERIFY(!controller.canWatch());
    QCOMPARE(controller.localLibraryErrorMessage(), QStringLiteral("Lookup failed"));

    episodes.failure.clear();
    episodes.result = {42, 1, QStringLiteral("Q:/Animes/Example/01.mkv")};
    opener.failure = QStringLiteral("Open failed");
    controller.SelectMedia(42);
    controller.WatchNext();
    QCOMPARE(controller.localLibraryErrorMessage(), QStringLiteral("Open failed"));
}

void HomeScreenControllerTests::refreshesNextLocalEpisodeAfterRecognitionCompletes() {
    FakeMediaReader reader;
    Media media; media.Id = 42; media.Name = QStringLiteral("Example"); media.Type = MediaType::Anime;
    reader.result = {media};
    FakeLocalEpisodeReader episodes;
    FakeLocalFileOpener opener;
    HomeScreenController controller(reader);
    controller.SetLocalEpisodeServices(&episodes, &opener);
    controller.reload(); controller.SelectMedia(42);
    QVERIFY(!controller.canWatch());
    QCOMPARE(controller.localLibraryStatusMessage(), QStringLiteral("Nenhum episódio disponível."));

    episodes.result = {42, 1, QStringLiteral("Q:/Animes/Example/01.mkv")};
    controller.RefreshLocalEpisode();

    QVERIFY(controller.canWatch());
    QCOMPARE(controller.nextLocalEpisode(), 1);
}

QTEST_MAIN(HomeScreenControllerTests)
#include "HomeScreenControllerTests.moc"
