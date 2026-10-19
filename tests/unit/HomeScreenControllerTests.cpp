#include <QFile>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

#include "../../src/presentation/home/HomeScreenController.h"
#include "../../src/application/library/ILocalFileOpener.h"
#include "../../src/application/library/ILocalEpisodeReader.h"
#include "../../src/application/media/IPersonalListChangeWriter.h"
#include "../../src/application/media/PersonalListChangeService.h"

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

class CapturingPersonalListChangeWriter final : public IPersonalListChangeWriter {
public:
    bool save(const Media &media, const QList<AniListPendingChange> &, QString &error) override {
        received = media;
        ++calls;
        error = failure;
        return failure.isEmpty();
    }
    bool remove(const Media &, const AniListPendingChange &, QString &error) override {
        error = QStringLiteral("Removal is not expected in this test.");
        return false;
    }

    Media received;
    QString failure;
    int calls = 0;
};

class RecordingPersonalListChangeWriter final : public IPersonalListChangeWriter {
public:
    bool remove(const Media &media, const AniListPendingChange &deletion, QString &error) override {
        removedMedia = media;
        removedDeletion = deletion;
        ++removeCalls;
        error = removeFailure;
        return removeFailure.isEmpty();
    }

    Media removedMedia;
    AniListPendingChange removedDeletion;
    QString removeFailure;
    int removeCalls = 0;
    bool save(const Media &media, const QList<AniListPendingChange> &changes,
              QString &error) override {
        savedMedia = media;
        savedChanges = changes;
        ++calls;
        error.clear();
        return true;
    }

    Media savedMedia;
    QList<AniListPendingChange> savedChanges;
    int calls = 0;
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
    bool openEpisode(const LocalEpisode &episode, QString &error) override {
        openedEpisode = episode;
        return open(episode.path, error);
    }
    LocalEpisode openedEpisode;
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
    QString AbsolutePath(const QString &path) const override { return QStringLiteral("C:/covers/") + path; }
};

class HomeScreenControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void exposesReadyMedia();
    void exposesEmptyState();
    void exposesErrorState();
    void exposesInitializationErrorWithoutRepository();
    void reloadAfterSynchronizationLoadsMedia();
    void doesNotExposeSynchronizationState();
    void errorStateComesOnlyFromReadFailure();
    void exposesPresentationReadyStatus_data();
    void exposesPresentationReadyStatus();
    void exposesSelfDescribingCardValues();
    void exposesPlaceholdersAndConfiguredScoreMaximumOnCards();
    void Should_ShowQuestionMarkProgress_When_TotalChaptersIsUnknown();
    void exposesNineItemPreviewAndCompleteFilteredLibrary();
    void reloadReflectsANewLocalPersonalListEntryInPreviewAndFullList();
    void reloadKeepsTheSelectedMediaWithItsNewData();
    void selectionExposesTheRecognizedLocalFolder();
    void currentCardSaysWatchingForAnimeAndReadingForManga();
    void reloadClearsTheSelectionWhenTheMediaIsGone();
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
    void Should_StartWithCurrentFilter_When_Created();
    void Should_FallbackToFirstOption_When_CurrentIsNotConfigured();
    void Should_ResetToCurrent_When_BrowseCriteriaCleared();
    void Should_ShowOnlyCurrentMedia_When_Started();
    void selectsMediaAndExposesItsDetails();
    void presentsCompleteExtendedMediaDetails();
    void presentsMissingExtendedMediaDetailsDeterministically();
    void keepsLongSynopsisAvailableForDetailsPresentation();
    void presentsNextAiringEpisodeAndTimestamp();
    void exposesControllerApprovedDeduplicatedMediaLinks();
    void fullMediaDetailsPanelProvidesSafeInteractiveDetails();
    void exposesConfigurableEditingOptions();
    void persistsSelectedMediaEditsAndRefreshesBothModels();
    void persistsSelectedMediaEditsToTheOfflineOutbox();
    void requestsFolderRecognitionWhenTheLocalPathChanges();
    void requestsPendingRecognitionWhenOnlyAlternativeNamesChange();
    void requestsNoRecognitionWhenNothingRelevantChanged();
    void requestsFolderRecognitionOnEverySaveWhenThePathIsUnchanged();
    void requestsTheOldFolderWhenThePathIsClearedOrMoved();
    void exposesSelectedLocalPathAfterSavingAndReloading();
    void keepsModelsAndSelectionUnchangedWhenPersistingSelectedMediaFails();
    void removesSelectedMediaFromLibraryWithoutRestart();
    void keepsMediaAndReportsOperationErrorWhenRemovalFails();
    void announcesSavedEditsAndRemovalsForTheHistory();
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

void HomeScreenControllerTests::currentCardSaysWatchingForAnimeAndReadingForManga() {
    FakeMediaReader reader;
    Media anime;
    anime.Id = 1;
    anime.Name = QStringLiteral("Frieren");
    anime.Type = MediaType::Anime;
    anime.ListStatus = UserListStatus::Current;
    Media manga = anime;
    manga.Id = 2;
    manga.Name = QStringLiteral("Berserk");
    manga.Type = MediaType::Manga;
    reader.result = {anime, manga};
    HomeScreenController controller(reader);
    controller.Reload();
    const auto label = [&controller] {
        return controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                             HomeMediaModel::StatusLabelRole).toString();
    };

    QCOMPARE(label(), QStringLiteral("Minha lista: Assistindo"));
    controller.SetMediaType(QStringLiteral("manga"));
    QCOMPARE(label(), QStringLiteral("Minha lista: Lendo"));
}

void HomeScreenControllerTests::selectionExposesTheRecognizedLocalFolder() {
    QTemporaryDir directory;
    FakeMediaReader reader;
    Media media;
    media.Id = 5;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.LocalPath = directory.path();
    reader.result = {media};
    HomeScreenController controller(reader);
    controller.Reload();

    QVERIFY(controller.selectedLocalFolderUrl().isEmpty());
    controller.SelectMedia(5);

    QCOMPARE(controller.selectedLocalFolderUrl(), QUrl::fromLocalFile(directory.path()).toString());
}

void HomeScreenControllerTests::reloadKeepsTheSelectedMediaWithItsNewData() {
    FakeMediaReader reader;
    Media media;
    media.Id = 5;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.ConsumedChapters = 3;
    reader.result = {media};
    HomeScreenController controller(reader);
    controller.Reload();
    controller.SelectMedia(5);

    // A synchronization finishes while the details are open and brings new progress.
    reader.result.first().ConsumedChapters = 4;
    controller.Reload();

    QVERIFY(controller.hasSelection());
    QCOMPARE(controller.selectedMediaId(), 5);
    QCOMPARE(controller.selectedProgressValue(), 4);
}

void HomeScreenControllerTests::reloadClearsTheSelectionWhenTheMediaIsGone() {
    FakeMediaReader reader;
    Media media;
    media.Id = 5;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    reader.result = {media};
    HomeScreenController controller(reader);
    controller.Reload();
    controller.SelectMedia(5);

    reader.result.clear();
    controller.Reload();

    QVERIFY(!controller.hasSelection());
}

void HomeScreenControllerTests::reloadReflectsANewLocalPersonalListEntryInPreviewAndFullList() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    controller.Reload();
    QCOMPARE(controller.mediaModel()->rowCount(), 0);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 0);

    Media added;
    added.Id = 77;
    added.Name = QStringLiteral("Seasonal addition");
    added.Type = MediaType::Anime;
    added.ListStatus = UserListStatus::Planning;
    reader.result.append(added);
    controller.Reload();

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
    media.ListStatus = UserListStatus::Current;
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
    controller.Reload();
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
    controller.Reload();

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

    controller.Reload();

    QCOMPARE(controller.state(), QStringLiteral("empty"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesErrorState() {
    FakeMediaReader reader;
    reader.failure = QStringLiteral("Database unavailable");
    HomeScreenController controller(reader);

    controller.Reload();

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Database unavailable"));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::exposesInitializationErrorWithoutRepository() {
    HomeScreenController controller(nullptr, QStringLiteral("Database initialization failed"));

    controller.Reload();

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Database initialization failed"));
    QCOMPARE(controller.statusMessage(), QStringLiteral("Não foi possível carregar os dados locais."));
    QCOMPARE(controller.mediaCount(), 0);
}

void HomeScreenControllerTests::reloadAfterSynchronizationLoadsMedia() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    reader.result.append(media);
    HomeScreenController controller(reader);

    controller.Reload();

    QCOMPARE(controller.state(), QStringLiteral("ready"));
    QCOMPARE(controller.mediaCount(), 1);
    QCOMPARE(controller.statusMessage(), QStringLiteral("Dados locais carregados."));
}

void HomeScreenControllerTests::doesNotExposeSynchronizationState() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);
    const QMetaObject *meta = controller.metaObject();

    QCOMPARE(meta->indexOfProperty("synchronizationProgress"), -1);
    QCOMPARE(meta->indexOfProperty("synchronizationProgressKnown"), -1);
    QCOMPARE(meta->indexOfSignal("synchronizationProgressChanged()"), -1);
}

void HomeScreenControllerTests::errorStateComesOnlyFromReadFailure() {
    FakeMediaReader reader;
    HomeScreenController healthy(reader);
    healthy.Reload();
    QCOMPARE(healthy.state(), QStringLiteral("empty"));
    QVERIFY(healthy.errorMessage().isEmpty());

    HomeScreenController failing(nullptr, QStringLiteral("Database initialization failed"));
    failing.Reload();
    QCOMPARE(failing.state(), QStringLiteral("error"));
    QCOMPARE(failing.errorMessage(), QStringLiteral("Database initialization failed"));
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
    media.ListStatus = UserListStatus::Current;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.ConfigureCardStatusPresentation(CardStatusPresentation::MediaReleaseStatus);

    controller.Reload();

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
        item.ListStatus = UserListStatus::Current;
        reader.result.append(item);
    }
    Media manga;
    manga.Id = 13;
    manga.Name = QStringLiteral("Manga");
    manga.Type = MediaType::Manga;
    manga.ListStatus = UserListStatus::Current;
    reader.result.append(manga);

    HomeScreenController controller(reader);
    controller.Reload();

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
    anime.ListStatus = UserListStatus::Current;
    Media manga;
    manga.Id = 2;
    manga.Name = QStringLiteral("Manga");
    manga.Type = MediaType::Manga;
    manga.ListStatus = UserListStatus::Current;
    Media novel;
    novel.Id = 3;
    novel.Name = QStringLiteral("Novel");
    novel.Type = MediaType::Novel;
    novel.ListStatus = UserListStatus::Current;
    reader.result = {anime, manga, novel};

    HomeScreenController controller(reader);
    controller.Reload();
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
    QCOMPARE(controller.activeListFilter(), QStringLiteral("current"));
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
    controller.SetListFilter(QStringLiteral("all"));

    controller.Reload();

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
    media.ListStatus = UserListStatus::Current;
    media.TotalChapters = 0;
    media.ConsumedChapters = 0;
    media.PersonalScore = 0;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.ConfigureScoreScale(0.0, 100.0, 5.0);

    controller.Reload();

    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ProgressRole).toString(),
             QStringLiteral("Progresso 0/?"));
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::ScoreRole).toString(),
             QStringLiteral("Nota —"));
    reader.result.first().PersonalScore = 85;
    controller.Reload();
    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                            HomeMediaModel::ScoreRole).toString(),
             QStringLiteral("Nota 85/100"));
}

void HomeScreenControllerTests::Should_ShowQuestionMarkProgress_When_TotalChaptersIsUnknown() {
    FakeMediaReader reader;
    Media media;
    media.Id = 44;
    media.Name = QStringLiteral("Unknown total");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.TotalChapters = 0;
    media.ConsumedChapters = 3;
    reader.result.append(media);
    HomeScreenController controller(reader);
    controller.Reload();
    controller.SelectMedia(44);

    QCOMPARE(controller.selectedProgress(), QStringLiteral("3/?"));
    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                            HomeMediaModel::ProgressRole).toString(),
             QStringLiteral("Progresso 3/?"));
}

void HomeScreenControllerTests::restoresConfiguredSortBeforeFirstModelPublication() {
    FakeMediaReader reader;
    Media first; first.Id = 1; first.Name = QStringLiteral("Alpha"); first.Type = MediaType::Anime;
    first.ListStatus = UserListStatus::Current;
    Media second; second.Id = 2; second.Name = QStringLiteral("Zulu"); second.Type = MediaType::Anime;
    second.ListStatus = UserListStatus::Current;
    reader.result = {first, second};
    HomeScreenController controller(reader);

    controller.ConfigureInitialSort(QStringLiteral("title_desc"));
    controller.Reload();

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
    controller.Reload();
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
    controller.Reload();
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
    winter.ListStatus = UserListStatus::Current;
    winter.Season = QStringLiteral("WINTER");
    winter.SeasonYear = 2024;
    Media spring;
    spring.Id = 2;
    spring.Name = QStringLiteral("Spring");
    spring.Type = MediaType::Anime;
    spring.ListStatus = UserListStatus::Current;
    spring.Season = QStringLiteral("SPRING");
    spring.SeasonYear = 2024;
    Media olderFall;
    olderFall.Id = 3;
    olderFall.Name = QStringLiteral("Older fall");
    olderFall.Type = MediaType::Anime;
    olderFall.ListStatus = UserListStatus::Current;
    olderFall.Season = QStringLiteral("FALL");
    olderFall.SeasonYear = 2023;
    Media undated;
    undated.Id = 4;
    undated.Name = QStringLiteral("Undated");
    undated.Type = MediaType::Anime;
    undated.ListStatus = UserListStatus::Current;
    reader.result = {spring, undated, olderFall, winter};

    HomeScreenController controller(reader);
    controller.ConfigureInitialSort(QStringLiteral("season_asc"));
    controller.Reload();

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
    first.ListStatus = UserListStatus::Current;
    Media second;
    second.Id = 43;
    second.Name = QStringLiteral("Z Romaji");
    second.EnglishName = QStringLiteral("Alpha English");
    second.OriginalName = QStringLiteral("ネイティブ Z");
    second.Type = MediaType::Anime;
    second.ListStatus = UserListStatus::Current;
    reader.result = {first, second};

    HomeScreenController controller(reader);
    controller.ConfigurePreferredTitle(QStringLiteral("english"));
    controller.Reload();

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
    controller.Reload();
    controller.SetMediaType(QStringLiteral("manga"));
    controller.SetListFilter(QStringLiteral("completed"));
    controller.SetSort(QStringLiteral("title_desc"));
    controller.SetSearchQuery(QStringLiteral("missing"));

    controller.ClearBrowseCriteria();

    QCOMPARE(controller.activeMediaType(), QStringLiteral("manga"));
    QCOMPARE(controller.activeListFilter(), QStringLiteral("current"));
    QCOMPARE(controller.activeSort(), QStringLiteral("title_asc"));
    QVERIFY(controller.searchQuery().isEmpty());
    QCOMPARE(controller.fullMediaModel()->rowCount(), 0);
}

void HomeScreenControllerTests::Should_StartWithCurrentFilter_When_Created() {
    FakeMediaReader reader;

    HomeScreenController controller(reader);

    QCOMPARE(controller.activeListFilter(), QStringLiteral("current"));
    QVERIFY(!controller.browseCriteriaActive());
}

void HomeScreenControllerTests::Should_FallbackToFirstOption_When_CurrentIsNotConfigured() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);

    controller.ConfigureBrowseOptions(
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("anime")},
                                 {QStringLiteral("label"), QStringLiteral("Anime")}}},
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("all")},
                                 {QStringLiteral("label"), QStringLiteral("All")}},
                     QVariantMap{{QStringLiteral("key"), QStringLiteral("planning")},
                                 {QStringLiteral("label"), QStringLiteral("Planning")}}},
        QVariantList{QVariantMap{{QStringLiteral("key"), QStringLiteral("title_asc")},
                                 {QStringLiteral("label"), QStringLiteral("Title")}}});

    QCOMPARE(controller.activeListFilter(), QStringLiteral("all"));
    QVERIFY(!controller.browseCriteriaActive());
}

void HomeScreenControllerTests::Should_ResetToCurrent_When_BrowseCriteriaCleared() {
    FakeMediaReader reader;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("planning"));
    QVERIFY(controller.browseCriteriaActive());

    controller.ClearBrowseCriteria();

    QCOMPARE(controller.activeListFilter(), QStringLiteral("current"));
    QVERIFY(!controller.browseCriteriaActive());
}

void HomeScreenControllerTests::Should_ShowOnlyCurrentMedia_When_Started() {
    FakeMediaReader reader;
    Media watching;
    watching.Id = 1;
    watching.Name = QStringLiteral("Frieren");
    watching.Type = MediaType::Anime;
    watching.ListStatus = UserListStatus::Current;
    Media completed;
    completed.Id = 2;
    completed.Name = QStringLiteral("Mushishi");
    completed.Type = MediaType::Anime;
    completed.ListStatus = UserListStatus::Completed;
    reader.result = {watching, completed};
    HomeScreenController controller(reader);

    controller.Reload();

    QCOMPARE(controller.filteredMediaCount(), 1);
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
    controller.Reload();

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
    controller.Reload();
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
    controller.Reload();
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
    controller.Reload();
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
    controller.Reload();
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
    controller.Reload();
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

void HomeScreenControllerTests::persistsSelectedMediaEditsAndRefreshesBothModels() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.ConsumedChapters = 5;
    media.PersonalScore = 7;
    reader.result = {media};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(media.Id);

    QVERIFY(controller.SaveSelectedMediaFromEditor(
        12, QStringLiteral("completed"), 9.0, QStringLiteral("D:/Anime/Frieren.mkv"),
        {QStringLiteral("Sousou no Frieren")}));

    QCOMPARE(writer.calls, 1);
    QCOMPARE(writer.received.Id, media.Id);
    QCOMPARE(writer.received.ConsumedChapters, 12);
    QCOMPARE(writer.received.PersonalScore, 9);
    QCOMPARE(writer.received.ListStatus, UserListStatus::Completed);
    QCOMPARE(writer.received.LocalPath, QStringLiteral("D:/Anime/Frieren.mkv"));
    QCOMPARE(controller.selectedProgressValue(), 12);
    QCOMPARE(controller.selectedScoreValue(), 9.0);
    QCOMPARE(controller.selectedListStatusKey(), QStringLiteral("completed"));
    QCOMPARE(controller.selectedListStatusLabel(), QStringLiteral("Concluídas"));
    QCOMPARE(controller.selectedAlternativeNames(), QStringList({QStringLiteral("Sousou no Frieren")}));
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);

    controller.SetListFilter(QStringLiteral("current"));
    QCOMPARE(controller.mediaModel()->rowCount(), 0);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 0);
    controller.SetListFilter(QStringLiteral("completed"));
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
}

void HomeScreenControllerTests::exposesSelectedLocalPathAfterSavingAndReloading() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    reader.result = {media};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    QVERIFY(controller.selectedLocalPath().isEmpty());
    controller.SelectMedia(media.Id);
    QVERIFY(controller.selectedLocalPath().isEmpty());

    QVERIFY(controller.SaveSelectedMediaFromEditor(
        1, QStringLiteral("current"), 0.0, QStringLiteral("D:/Anime/Frieren"), {}));
    QCOMPARE(controller.selectedLocalPath(), QStringLiteral("D:/Anime/Frieren"));

    // Simulates a restart: the repository now returns the persisted path.
    reader.result = {writer.received};
    controller.Reload();
    controller.SelectMedia(media.Id);
    QCOMPARE(controller.selectedLocalPath(), QStringLiteral("D:/Anime/Frieren"));
}

namespace {
Media RecognitionMedia(const QString &path = QString(), const QStringList &names = {}) {
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.LocalPath = path;
    media.AlternativeNames = names;
    return media;
}
}

void HomeScreenControllerTests::requestsFolderRecognitionWhenTheLocalPathChanges() {
    FakeMediaReader reader;
    reader.result = {RecognitionMedia(QStringLiteral("D:/Old"))};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(42);
    QSignalSpy requested(&controller, &HomeScreenController::localRecognitionRequested);

    QVERIFY(controller.SaveSelectedMediaFromEditor(0, QStringLiteral("current"), 0.0,
                                                    QStringLiteral("D:/Anime/Frieren"), {}));

    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.first().at(0).toStringList(),
             (QStringList{QStringLiteral("D:/Anime/Frieren"), QStringLiteral("D:/Old")}));
    QCOMPARE(requested.first().at(1).toBool(), false);
}

void HomeScreenControllerTests::requestsPendingRecognitionWhenOnlyAlternativeNamesChange() {
    FakeMediaReader reader;
    reader.result = {RecognitionMedia(QString(), {QStringLiteral("Old")})};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(42);
    QSignalSpy requested(&controller, &HomeScreenController::localRecognitionRequested);

    QVERIFY(controller.SaveSelectedMediaFromEditor(0, QStringLiteral("current"), 0.0, QString(),
                                                    {QStringLiteral("New")}));

    QCOMPARE(requested.count(), 1);
    QVERIFY(requested.first().at(0).toStringList().isEmpty());
    QCOMPARE(requested.first().at(1).toBool(), true);
}

void HomeScreenControllerTests::requestsNoRecognitionWhenNothingRelevantChanged() {
    FakeMediaReader reader;
    reader.result = {RecognitionMedia(QString(), {QStringLiteral("Same")})};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(42);
    QSignalSpy requested(&controller, &HomeScreenController::localRecognitionRequested);

    QVERIFY(controller.SaveSelectedMediaFromEditor(9, QStringLiteral("completed"), 8.0, QString(),
                                                    {QStringLiteral("Same")}));
    // A failed save never requests recognition.
    writer.failure = QStringLiteral("disk unavailable");
    QVERIFY(!controller.SaveSelectedMediaFromEditor(9, QStringLiteral("completed"), 8.0,
                                                     QStringLiteral("D:/Other"), {QStringLiteral("Other")}));

    QCOMPARE(requested.count(), 0);
}

void HomeScreenControllerTests::requestsFolderRecognitionOnEverySaveWhenThePathIsUnchanged() {
    FakeMediaReader reader;
    reader.result = {RecognitionMedia(QStringLiteral("D:/Anime/Frieren"), {QStringLiteral("Same")})};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(42);
    QSignalSpy requested(&controller, &HomeScreenController::localRecognitionRequested);

    QVERIFY(controller.SaveSelectedMediaFromEditor(9, QStringLiteral("completed"), 8.0,
                                                    QStringLiteral("D:/Anime/Frieren"),
                                                    {QStringLiteral("Same")}));

    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.first().at(0).toStringList(), QStringList{QStringLiteral("D:/Anime/Frieren")});
    QCOMPARE(requested.first().at(1).toBool(), false);
}

void HomeScreenControllerTests::requestsTheOldFolderWhenThePathIsClearedOrMoved() {
    FakeMediaReader reader;
    reader.result = {RecognitionMedia(QStringLiteral("D:/Old"))};
    CapturingPersonalListChangeWriter writer;
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(42);
    QSignalSpy requested(&controller, &HomeScreenController::localRecognitionRequested);

    QVERIFY(controller.SaveSelectedMediaFromEditor(0, QStringLiteral("current"), 0.0, QString(), {}));

    QCOMPARE(requested.count(), 1);
    QCOMPARE(requested.first().at(0).toStringList(), QStringList{QStringLiteral("D:/Old")});
}

void HomeScreenControllerTests::persistsSelectedMediaEditsToTheOfflineOutbox() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.ConsumedChapters = 5;
    media.PersonalScore = 7;
    reader.result = {media};
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService offlineChanges(writer);
    HomeScreenController controller(reader);
    controller.SetListFilter(QStringLiteral("all"));
    controller.SetPersonalListChangeService(&offlineChanges);
    controller.Reload();
    controller.SelectMedia(media.Id);

    QVERIFY(controller.SaveSelectedMediaFromEditor(12, QStringLiteral("completed"), 9.0,
                                                    QStringLiteral("D:/Anime/Frieren.mkv"), {}));

    QCOMPARE(writer.calls, 1);
    QCOMPARE(writer.savedMedia.ConsumedChapters, 12);
    QCOMPARE(writer.savedChanges.size(), 3);
    QCOMPARE(controller.selectedProgressValue(), 12);
    QCOMPARE(controller.selectedListStatusKey(), QStringLiteral("completed"));
}

void HomeScreenControllerTests::keepsModelsAndSelectionUnchangedWhenPersistingSelectedMediaFails() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.ConsumedChapters = 5;
    media.PersonalScore = 7;
    reader.result = {media};
    CapturingPersonalListChangeWriter writer;
    writer.failure = QStringLiteral("disk unavailable");
    HomeScreenController controller(reader);
    PersonalListChangeService changes(writer);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(media.Id);

    QVERIFY(!controller.SaveSelectedMediaFromEditor(
        12, QStringLiteral("completed"), 9.0, QStringLiteral("D:/Anime/Frieren.mkv"), {}));

    QCOMPARE(writer.calls, 1);
    QCOMPARE(controller.selectedMediaId(), media.Id);
    QCOMPARE(controller.selectedProgressValue(), 5);
    QCOMPARE(controller.selectedScoreValue(), 7.0);
    QCOMPARE(controller.selectedListStatusKey(), QStringLiteral("current"));
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    QCOMPARE(controller.mediaOperationErrorMessage(), QStringLiteral("disk unavailable"));
    QVERIFY(controller.errorMessage().isEmpty());
}

void HomeScreenControllerTests::removesSelectedMediaFromLibraryWithoutRestart() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    Media other = media;
    other.Id = 43;
    other.Name = QStringLiteral("Other");
    reader.result = {media, other};
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService changes(writer);
    HomeScreenController controller(reader);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(media.Id);
    QCOMPARE(controller.mediaCount(), 2);
    QSignalSpy countSpy(&controller, &HomeScreenController::mediaCountChanged);

    QVERIFY(controller.RemoveSelectedMedia());

    QCOMPARE(writer.removeCalls, 1);
    QCOMPARE(writer.removedMedia.Id, 42);
    QVERIFY(!controller.hasSelection());
    QCOMPARE(controller.mediaCount(), 1);
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QVERIFY(countSpy.count() >= 1);
    QVERIFY(controller.errorMessage().isEmpty());
    QVERIFY(controller.mediaOperationErrorMessage().isEmpty());
}

void HomeScreenControllerTests::keepsMediaAndReportsOperationErrorWhenRemovalFails() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    reader.result = {media};
    RecordingPersonalListChangeWriter writer;
    writer.removeFailure = QStringLiteral("disk unavailable");
    PersonalListChangeService changes(writer);
    HomeScreenController controller(reader);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(media.Id);

    QVERIFY(!controller.RemoveSelectedMedia());

    QVERIFY(controller.hasSelection());
    QCOMPARE(controller.mediaCount(), 1);
    QCOMPARE(controller.mediaOperationErrorMessage(), QStringLiteral("disk unavailable"));
    QVERIFY(controller.errorMessage().isEmpty());
}

void HomeScreenControllerTests::announcesSavedEditsAndRemovalsForTheHistory() {
    // Arrange
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
    media.ConsumedChapters = 3;
    reader.result = {media};
    RecordingPersonalListChangeWriter writer;
    PersonalListChangeService changes(writer);
    HomeScreenController controller(reader);
    controller.SetPersonalListChangeService(&changes);
    controller.Reload();
    controller.SelectMedia(media.Id);
    QList<std::pair<Media, Media>> edits;
    QList<Media> removals;
    connect(&controller, &HomeScreenController::personalListEdited, this,
            [&edits](const Media &previous, const Media &updated) { edits.append({previous, updated}); });
    connect(&controller, &HomeScreenController::personalListRemoved, this,
            [&removals](const Media &removed) { removals.append(removed); });

    // Act
    QVERIFY(controller.SaveSelectedMediaFromEditor(5, QStringLiteral("current"), 0.0, QString(), {}));
    QVERIFY(controller.RemoveSelectedMedia());

    // Assert
    QCOMPARE(edits.size(), 1);
    QCOMPARE(edits.first().first.ConsumedChapters, 3);
    QCOMPARE(edits.first().second.ConsumedChapters, 5);
    QCOMPARE(removals.size(), 1);
    QCOMPARE(removals.first().Id, 42);
    QCOMPARE(removals.first().ConsumedChapters, 5);
}

void HomeScreenControllerTests::selectedCoverFallsBackWhenCachedFileIsMissing() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Current;
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
    controller.Reload();
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
    model.SetMedia({first, second});
    const auto index = model.index(0, 0);
    QCOMPARE(model.data(index, HomeMediaModel::CoverSourceRole).toString(),
             QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"));
    QCOMPARE(model.data(index, HomeMediaModel::RemoteCoverUrlRole).toString(), first.CoverUrl);
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy reset(&model, &QAbstractItemModel::modelReset);
    QVERIFY(model.UpdateCover(42, QStringLiteral("file:///covers/42.jpg"), CoverState::Available));
    QCOMPARE(changed.count(), 1); QCOMPARE(reset.count(), 0);
    QCOMPARE(model.CoverSource(42), QStringLiteral("file:///covers/42.jpg"));
    QVERIFY(model.CoverSource(7).isEmpty());
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
    controller.Reload(); controller.SelectMedia(42);
    QVERIFY(controller.canWatch());
    QCOMPARE(controller.nextLocalEpisode(), 3);
    QCOMPARE(episodes.requestedMediaId, 42);
    QCOMPARE(episodes.requestedConsumedEpisode, 2);
    controller.WatchNext();
    QCOMPARE(opener.openedPath, QStringLiteral("Q:/Animes/Example/03.mkv"));
    QCOMPARE(opener.openedEpisode.mediaId, 42);
    QCOMPARE(opener.openedEpisode.episode, 3);
    QCOMPARE(opener.openedEpisode.path, QStringLiteral("Q:/Animes/Example/03.mkv"));
}

void HomeScreenControllerTests::reportsLocalEpisodeLookupAndOpenFailures() {
    FakeMediaReader reader;
    Media media; media.Id = 42; media.Name = QStringLiteral("Example"); media.Type = MediaType::Anime;
    reader.result = {media};
    FakeLocalEpisodeReader episodes; episodes.failure = QStringLiteral("Lookup failed");
    FakeLocalFileOpener opener;
    HomeScreenController controller(reader);
    controller.SetLocalEpisodeServices(&episodes, &opener);
    controller.Reload(); controller.SelectMedia(42);
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
    controller.Reload(); controller.SelectMedia(42);
    QVERIFY(!controller.canWatch());
    QCOMPARE(controller.localLibraryStatusMessage(), QStringLiteral("Nenhum episódio disponível."));

    episodes.result = {42, 1, QStringLiteral("Q:/Animes/Example/01.mkv")};
    controller.RefreshLocalEpisode();

    QVERIFY(controller.canWatch());
    QCOMPARE(controller.nextLocalEpisode(), 1);
}

QTEST_MAIN(HomeScreenControllerTests)
#include "HomeScreenControllerTests.moc"
