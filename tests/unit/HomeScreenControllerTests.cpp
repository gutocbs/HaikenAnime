#include <QtTest>

#include "../../src/presentation/home/HomeScreenController.h"

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
    bool Clear(QString &) override { entries.clear(); return true; }
    QHash<int, CoverCacheEntry> entries;
};

class ExistingCoverFiles final : public ICoverFileStore {
public:
    bool Exists(const QString &path) const override { return path == QStringLiteral("42.jpg"); }
    bool Publish(int, const QString &, const QString &, const QString &, PublishedCover &, QString &) override { return false; }
    bool Remove(const QString &, QString &) override { return true; }
    bool Clear(QString &) override { return true; }
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
    void exposesNineItemPreviewAndCompleteFilteredLibrary();
    void changesMediaTypeUsingStableKeys();
    void exposesBackendDrivenBrowseOptions();
    void reconcilesActiveCriteriaWhenBackendOptionsChange();
    void appliesListSearchAndSortToBothLibraries();
    void searchesAllKnownTitlesCaseInsensitively();
    void clearsBrowseCriteriaWithoutChangingMediaType();
    void selectsMediaAndExposesItsDetails();
    void exposesConfigurableEditingOptions();
    void selectedCoverFallsBackWhenCachedFileIsMissing();
    void updatesOnlyOneCoverRowAndPreservesOldCoverOnFailure();
    void coverQualityChangesFutureRequestsWithoutClearingDisplayedCover();
};

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
             QStringLiteral("12/28"));
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
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Frieren");
    media.Status = static_cast<MediaStatus>(status);
    media.Type = MediaType::Anime;
    reader.result.append(media);
    HomeScreenController controller(reader);

    controller.reload();

    const auto index = controller.mediaModel()->index(0, 0);
    QCOMPARE(controller.mediaModel()->data(index, HomeMediaModel::StatusLabelRole).toString(),
             expectedLabel);
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

void HomeScreenControllerTests::searchesAllKnownTitlesCaseInsensitively() {
    FakeMediaReader reader;
    Media media;
    media.Id = 42;
    media.Name = QStringLiteral("Sousou no Frieren");
    media.EnglishName = QStringLiteral("Frieren: Beyond Journey's End");
    media.OriginalName = QStringLiteral("葬送のフリーレン");
    media.AlternativeNames = {QStringLiteral("Frieren at the Funeral")};
    media.Type = MediaType::Anime;
    media.ListStatus = UserListStatus::Planning;
    reader.result.append(media);

    HomeScreenController controller(reader);
    controller.reload();
    controller.SetSearchQuery(QStringLiteral("BEYOND JOURNEY"));

    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    controller.SetSearchQuery(QStringLiteral("funeral"));
    QCOMPARE(controller.fullMediaModel()->rowCount(), 1);
    controller.SetSearchQuery(QStringLiteral("not present"));
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

QTEST_MAIN(HomeScreenControllerTests)
#include "HomeScreenControllerTests.moc"
