#include <QtTest>

#include "../../src/app/SeasonalCatalogCoordinator.h"
#include "../../src/application/catalog/ISeasonalCatalogDataSource.h"
#include "../../src/application/media/SeasonalPersonalListService.h"
#include "../../src/presentation/seasonal/SeasonalCatalogController.h"

namespace {
Media media(const int id, const QString &name = {}) {
    Media value;
    value.Id = id;
    value.Name = name.isEmpty() ? QStringLiteral("Media %1").arg(id) : name;
    value.EnglishName = QStringLiteral("English %1").arg(id);
    value.CoverLargeUrl = QStringLiteral("https://example.test/%1.jpg").arg(id);
    value.Synopsis = QStringLiteral("Synopsis %1").arg(id);
    value.Season = QStringLiteral("SPRING");
    value.SeasonYear = 2026;
    return value;
}

MediaPage page(const int currentPage, const QList<Media> &media, const bool hasNextPage = false) {
    MediaPage value;
    value.currentPage = currentPage;
    value.totalPages = 2;
    value.hasNextPage = hasNextPage;
    value.media = media;
    return value;
}

class RecordingSource final : public ISeasonalCatalogDataSource {
public:
    bool Fetch(const SeasonalCatalogRequest &request, MediaPage &result, QString &error) override {
        requests.append(request);
        if (onFetch) onFetch(request);
        if (!failure.isEmpty()) {
            error = failure;
            return false;
        }
        result = responses.value(request.page);
        return true;
    }

    QList<SeasonalCatalogRequest> requests;
    QHash<int, MediaPage> responses;
    QString failure;
    std::function<void(const SeasonalCatalogRequest &)> onFetch;
};

class InMemoryMediaRepository final : public IMediaReader, public IMediaWriter,
                                      public IPersonalListMediaWriter {
public:
    bool readAll(QList<Media> &result, QString &error) override {
        result = media;
        error.clear();
        return true;
    }

    bool upsert(const QList<Media> &items, QString &error) override {
        if (!failure.isEmpty()) {
            error = failure;
            return false;
        }
        for (const Media &item : items) {
            const auto found = std::find_if(media.begin(), media.end(), [&item](const Media &existing) {
                return existing.Id == item.Id;
            });
            if (found == media.end()) media.append(item);
            else *found = item;
        }
        ++upsertCalls;
        error.clear();
        return true;
    }

    bool updatePersonalListMedia(const Media &item, QString &error) override {
        if (!failure.isEmpty()) {
            error = failure;
            return false;
        }
        const auto found = std::find_if(media.begin(), media.end(), [&item](const Media &existing) {
            return existing.Id == item.Id;
        });
        if (found == media.end()) {
            error = QStringLiteral("Local media record was not found.");
            return false;
        }
        *found = item;
        ++personalListUpdateCalls;
        error.clear();
        return true;
    }

    QList<Media> media;
    QString failure;
    int upsertCalls = 0;
    int personalListUpdateCalls = 0;
};
}

class SeasonalCatalogControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void doesNotRequestUntilBothFiltersAreExplicitlySelected();
    void exposesBackendOwnedStableFilterOptionsWithoutDefaults();
    void exposesResultsSelectionAndPagingCommands();
    void exposesExplicitAniListScoreLabel();
    void retainsResultsAndSuppressesDuplicateLoadsWhileAppending();
    void adultPolicyChangeReloadsActiveFiltersWithoutMixingPriorResults();
    void exposesHomeCompatibleDetailsPresentation();
    void exposesErrorAndRetriesTheCurrentSelection();
    void blocksAnAbsentSelectionUntilOnePersonalStatusIsChosen();
    void addsAbsentSelectionIdempotentlyAndKeepsCatalogMetadata();
    void reusesAnExistingLocalEntryWithoutOverwritingUserFields();
    void retainsTheDraftWhenLocalPersistenceFails();
};

void SeasonalCatalogControllerTests::doesNotRequestUntilBothFiltersAreExplicitlySelected() {
    RecordingSource source;
    source.responses.insert(1, page(1, {media(7)}, true));
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);

    QCOMPARE(controller.selectedYear(), 0);
    QVERIFY(controller.selectedSeasonKey().isEmpty());
    QCOMPARE(source.requests.size(), 0);

    controller.SetYear(2026);
    QCOMPARE(source.requests.size(), 0);
    controller.SetSeason(QStringLiteral("SPRING"));

    QCOMPARE(source.requests.size(), 1);
    QCOMPARE(controller.state(), QStringLiteral("populated"));
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QVERIFY(controller.canLoadNextPage());
}

void SeasonalCatalogControllerTests::exposesBackendOwnedStableFilterOptionsWithoutDefaults() {
    RecordingSource source;
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);

    QCOMPARE(controller.availableYearOptions(), coordinator.availableYearOptions());
    QCOMPARE(controller.availableSeasonOptions(), coordinator.availableSeasonOptions());
    QCOMPARE(controller.selectedYear(), 0);
    QVERIFY(controller.selectedSeasonKey().isEmpty());
    QCOMPARE(source.requests.size(), 0);
}

void SeasonalCatalogControllerTests::exposesResultsSelectionAndPagingCommands() {
    RecordingSource source;
    source.responses.insert(1, page(1, {media(7, QStringLiteral("Romaji"))}, true));
    source.responses.insert(2, page(2, {media(8)}));
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);
    controller.ConfigurePreferredTitle(QStringLiteral("english"));
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));

    controller.SelectMedia(7);
    QCOMPARE(controller.selectedTitle(), QStringLiteral("English 7"));
    QCOMPARE(controller.selectedCoverSource(), QStringLiteral("https://example.test/7.jpg"));
    QCOMPARE(controller.selectedSynopsis(), QStringLiteral("Synopsis 7"));

    controller.LoadNextPage();
    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(controller.mediaModel()->rowCount(), 2);
}

void SeasonalCatalogControllerTests::exposesExplicitAniListScoreLabel() {
    RecordingSource source;
    auto item = media(7);
    item.AverageScore = 72;
    source.responses.insert(1, page(1, {item}));
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);

    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));

    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                           SeasonalCatalogMediaModel::ScoreRole),
             QStringLiteral("Nota AniList: 72"));
}

void SeasonalCatalogControllerTests::retainsResultsAndSuppressesDuplicateLoadsWhileAppending() {
    RecordingSource source;
    source.responses.insert(1, page(1, {media(7)}, true));
    source.responses.insert(2, page(2, {media(8)}));
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);
    bool sawResultsWhileLoading = false;
    source.onFetch = [&controller, &sawResultsWhileLoading](const SeasonalCatalogRequest &request) {
        if (request.page != 2) return;
        sawResultsWhileLoading = controller.state() == QStringLiteral("loading") && controller.hasResults();
        controller.LoadNextPage();
    };

    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    controller.LoadNextPage();
    controller.LoadNextPage();

    QVERIFY(sawResultsWhileLoading);
    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(controller.mediaModel()->rowCount(), 2);
    QVERIFY(!controller.canLoadNextPage());
}

void SeasonalCatalogControllerTests::adultPolicyChangeReloadsActiveFiltersWithoutMixingPriorResults() {
    RecordingSource source;
    source.responses.insert(1, page(1, {media(7)}));
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);

    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    source.responses.insert(1, page(1, {media(8)}));
    controller.ConfigureIncludeAdultContent(true);

    QCOMPARE(source.requests.size(), 2);
    QVERIFY(!source.requests.at(0).includeAdultContent);
    QVERIFY(source.requests.at(1).includeAdultContent);
    QCOMPARE(controller.mediaModel()->rowCount(), 1);
    QCOMPARE(controller.mediaModel()->data(controller.mediaModel()->index(0, 0),
                                           SeasonalCatalogMediaModel::MediaIdRole).toInt(), 8);
}

void SeasonalCatalogControllerTests::exposesHomeCompatibleDetailsPresentation() {
    RecordingSource source;
    Media selected = media(7);
    selected.Season = QStringLiteral("spring");
    selected.NextAiringEpisode = 4;
    selected.NextAiringAt = 0;
    selected.AniListUrl = QStringLiteral("https://anilist.co/media/7");
    selected.ExternalLinks = {{QStringLiteral("Official"), QStringLiteral("https://example.test/official")},
                              {QStringLiteral("official"), QStringLiteral("https://example.test/official")},
                              {QStringLiteral(""), QStringLiteral("https://example.test/invalid")}};
    source.responses.insert(1, page(1, {selected}));
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    controller.SelectMedia(7);

    QCOMPARE(controller.selectedSeasonLabel(), QStringLiteral("Primavera de 2026"));
    QCOMPARE(controller.selectedNextAiringLabel(), QStringLiteral("Episódio 4 · 01/01/1970 00:00 UTC"));
    const auto links = controller.selectedMediaLinks();
    QCOMPARE(links.size(), 2);
    QCOMPARE(links.at(0).toMap().value(QStringLiteral("site")).toString(), QStringLiteral("AniList"));
    QCOMPARE(links.at(1).toMap().value(QStringLiteral("site")).toString(), QStringLiteral("Official"));
}

void SeasonalCatalogControllerTests::exposesErrorAndRetriesTheCurrentSelection() {
    RecordingSource source;
    source.failure = QStringLiteral("Network unavailable");
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator);
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));

    QCOMPARE(controller.state(), QStringLiteral("error"));
    QCOMPARE(controller.errorMessage(), QStringLiteral("Network unavailable"));

    source.failure.clear();
    source.responses.insert(1, page(1, {media(9)}));
    controller.Retry();

    QCOMPARE(source.requests.size(), 2);
    QCOMPARE(controller.state(), QStringLiteral("populated"));
}

void SeasonalCatalogControllerTests::blocksAnAbsentSelectionUntilOnePersonalStatusIsChosen() {
    RecordingSource source;
    Media selected = media(7);
    source.responses.insert(1, page(1, {selected}));
    InMemoryMediaRepository repository;
    SeasonalPersonalListService personalLists(&repository, &repository, &repository);
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator, CoverQuality::Medium,
                                         DefaultPreferredTitleKey(), nullptr, &personalLists);
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    controller.SelectMedia(selected.Id);

    QVERIFY(!controller.selectedMediaInPersonalList());
    QVERIFY(!controller.availablePersonalListOptions().isEmpty());
    QVERIFY(!controller.SaveSelectedToPersonalList(0, QString(), 0.0, QString(), {}));
    QCOMPARE(repository.upsertCalls, 0);
    QVERIFY(!controller.personalListErrorMessage().isEmpty());
    QCOMPARE(controller.selectedMediaId(), selected.Id);
}

void SeasonalCatalogControllerTests::addsAbsentSelectionIdempotentlyAndKeepsCatalogMetadata() {
    RecordingSource source;
    Media selected = media(7, QStringLiteral("Romaji"));
    selected.AniListUrl = QStringLiteral("https://anilist.co/anime/7");
    selected.ExternalLinks = {{QStringLiteral("Official"), QStringLiteral("https://example.test/official")}};
    source.responses.insert(1, page(1, {selected}));
    InMemoryMediaRepository repository;
    SeasonalPersonalListService personalLists(&repository, &repository, &repository);
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator, CoverQuality::Medium,
                                         DefaultPreferredTitleKey(), nullptr, &personalLists);
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    controller.SelectMedia(selected.Id);

    QVERIFY(controller.SaveSelectedToPersonalList(0, QStringLiteral("planning"), 0.0, QString(), {}));
    QVERIFY(controller.selectedMediaInPersonalList());
    QCOMPARE(repository.upsertCalls, 1);
    QCOMPARE(repository.media.size(), 1);
    QCOMPARE(repository.media.first().Name, selected.Name);
    QCOMPARE(repository.media.first().EnglishName, selected.EnglishName);
    QCOMPARE(repository.media.first().CoverLargeUrl, selected.CoverLargeUrl);
    QCOMPARE(repository.media.first().Synopsis, selected.Synopsis);
    QCOMPARE(repository.media.first().AniListUrl, selected.AniListUrl);
    QCOMPARE(repository.media.first().ExternalLinks.first().Url, selected.ExternalLinks.first().Url);
    QCOMPARE(repository.media.first().ListStatus, UserListStatus::Planning);

    QVERIFY(controller.SaveSelectedToPersonalList(0, QStringLiteral("planning"), 0.0, QString(), {}));
    QCOMPARE(repository.media.first().ListStatus, UserListStatus::Planning);
    QCOMPARE(repository.media.size(), 1);
    QCOMPARE(repository.upsertCalls, 1);
    QCOMPARE(repository.personalListUpdateCalls, 1);
}

void SeasonalCatalogControllerTests::reusesAnExistingLocalEntryWithoutOverwritingUserFields() {
    RecordingSource source;
    Media selected = media(7, QStringLiteral("Catalog title"));
    source.responses.insert(1, page(1, {selected}));
    InMemoryMediaRepository repository;
    Media existing = selected;
    existing.ConsumedChapters = 6;
    existing.PersonalScore = 9;
    existing.NextChapter = 7;
    existing.LocalPath = QStringLiteral("D:\\Original\\Frieren");
    existing.AlternativeNames = {QStringLiteral("My custom title")};
    existing.ListStatus = UserListStatus::Current;
    repository.media.append(existing);
    SeasonalPersonalListService personalLists(&repository, &repository, &repository);
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator, CoverQuality::Medium,
                                         DefaultPreferredTitleKey(), nullptr, &personalLists);
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    controller.SelectMedia(selected.Id);

    QVERIFY(controller.selectedMediaInPersonalList());
    QCOMPARE(controller.selectedProgressValue(), 6);
    QCOMPARE(controller.selectedScoreValue(), 9.0);
    QCOMPARE(controller.selectedLocalPath(), existing.LocalPath);
    QCOMPARE(controller.selectedAlternativeNames(), existing.AlternativeNames);
    QCOMPARE(controller.selectedListStatusKey(), QStringLiteral("current"));
    const QString qmlAlternativeNames = QStringLiteral("  Edited title ; ; Localized title ;  ");
    const QStringList editedAlternativeNames{QStringLiteral("Edited title"), QStringLiteral("Localized title")};
    QVERIFY(controller.SaveSelectedToPersonalListFromEditor(8, QStringLiteral("completed"), 7.0,
                                                            QStringLiteral("C:\\Media\\Frieren"),
                                                            qmlAlternativeNames));
    QCOMPARE(repository.upsertCalls, 0);
    QCOMPARE(repository.personalListUpdateCalls, 1);
    QCOMPARE(repository.media.first().ConsumedChapters, 8);
    QCOMPARE(repository.media.first().PersonalScore, 7);
    QCOMPARE(repository.media.first().ListStatus, UserListStatus::Completed);
    QCOMPARE(repository.media.first().AlternativeNames, editedAlternativeNames);
    QCOMPARE(repository.media.first().LocalPath, QStringLiteral("C:\\Media\\Frieren"));
    QCOMPARE(repository.media.first().NextChapter, existing.NextChapter);
}

void SeasonalCatalogControllerTests::retainsTheDraftWhenLocalPersistenceFails() {
    RecordingSource source;
    Media selected = media(7);
    source.responses.insert(1, page(1, {selected}));
    InMemoryMediaRepository repository;
    repository.failure = QStringLiteral("disk full");
    SeasonalPersonalListService personalLists(&repository, &repository, &repository);
    SeasonalCatalogCoordinator coordinator(source);
    SeasonalCatalogController controller(&coordinator, CoverQuality::Medium,
                                         DefaultPreferredTitleKey(), nullptr, &personalLists);
    controller.SetYear(2026);
    controller.SetSeason(QStringLiteral("SPRING"));
    controller.SelectMedia(selected.Id);

    QVERIFY(!controller.SaveSelectedToPersonalList(0, QStringLiteral("planning"), 0.0, QString(), {}));
    QCOMPARE(controller.selectedMediaId(), selected.Id);
    QVERIFY(!controller.selectedMediaInPersonalList());
    QCOMPARE(controller.personalListErrorMessage(), QStringLiteral("disk full"));
}

QTEST_GUILESS_MAIN(SeasonalCatalogControllerTests)
#include "SeasonalCatalogControllerTests.moc"
