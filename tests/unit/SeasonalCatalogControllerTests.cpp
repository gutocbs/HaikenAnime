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
};
}

class SeasonalCatalogControllerTests final : public QObject {
    Q_OBJECT

private slots:
    void doesNotRequestUntilBothFiltersAreExplicitlySelected();
    void exposesResultsSelectionAndPagingCommands();
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
