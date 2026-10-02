#include <QtTest>

#include "../../src/app/SeasonalCatalogCoordinator.h"
#include "../../src/application/catalog/ISeasonalCatalogDataSource.h"
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

QTEST_GUILESS_MAIN(SeasonalCatalogControllerTests)
#include "SeasonalCatalogControllerTests.moc"
