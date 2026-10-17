#include <QElapsedTimer>
#include <QtTest>

#include "../../src/application/library/LocalMediaResolver.h"

namespace {
LocalFileRecognition Recognized(const QString &title, const std::optional<int> episode = 1) {
    LocalFileRecognition recognition;
    recognition.state = LocalRecognitionState::Recognized;
    recognition.mediaKind = LocalMediaKind::Anime;
    recognition.extractedTitle = title;
    recognition.episode = episode;
    return recognition;
}

Media CatalogMedia(const int id, const QString &name) {
    Media media;
    media.Id = id;
    media.Name = name;
    return media;
}
}

class LocalMediaResolverTests final : public QObject {
    Q_OBJECT

private slots:
    void matchesPrimaryTitle();
    void matchesEnglishAndOriginalTitles_data();
    void matchesEnglishAndOriginalTitles();
    void matchesAlternativeTitle();
    void matchesPersistedSynonym();
    void normalizesCaseWhitespaceAndEquivalentSeparators();
    void returnsUnrecognizedForUnknownTitle();
    void doesNotUseFuzzyMatching();
    void returnsAmbiguousForMultipleCatalogMatches();
    void doesNotTreatMultipleVariantsOfOneMediaAsAmbiguous();
    void ignoresPunctuationWhenMatchingEnglishName();
    void ignoresPunctuationWhenExtractedTitleIsDotted();
    void ignoresPunctuationWhenMatchingAlternativeName();
    void treatsColonAndSpaceAsEquivalent();
    void returnsAmbiguousWhenPunctuationOnlyDistinguishesMedia();
    void dropsApostrophesInsteadOfSplittingWords();
    void stillMatchesNonLatinTitles();
    void rejectsUnsupportedKind();
    void rejectsInvalidEpisode_data();
    void rejectsInvalidEpisode();
    void indexedResolveMatchesCatalogResolveForEveryFixtureTitle_data();
    void indexedResolveMatchesCatalogResolveForEveryFixtureTitle();
    void indexCountsOneMediaOnceEvenWhenSeveralNamesCollapseToTheSameTitle();
    void indexedResolveOfLargeCatalogStaysFast();
    void folderAssociationWinsOverTitleMatch();
    void folderMatchRequiresFolderBoundary();
    void deepestFolderWins();
    void folderMatchIgnoresSeparatorStyleAndCaseOnWindows();
    void folderMatchWithoutValidEpisodeIsAmbiguous();
    void unrelatedFolderFallsBackToTitleMatching();
};

void LocalMediaResolverTests::matchesPrimaryTitle() {
    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Frieren")),
        {CatalogMedia(10, QStringLiteral("Frieren"))});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 10);
    QVERIFY(match.diagnostic.isEmpty());
}

void LocalMediaResolverTests::matchesEnglishAndOriginalTitles_data() {
    QTest::addColumn<QString>("title");

    QTest::newRow("english") << QStringLiteral("Frieren: Beyond Journey's End");
    QTest::newRow("original") << QStringLiteral("Sousou no Frieren");
}

void LocalMediaResolverTests::matchesEnglishAndOriginalTitles() {
    QFETCH(QString, title);
    auto media = CatalogMedia(20, QStringLiteral("Frieren"));
    media.EnglishName = QStringLiteral("Frieren: Beyond Journey's End");
    media.OriginalName = QStringLiteral("Sousou no Frieren");

    const auto match = LocalMediaResolver().resolve(Recognized(title), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 20);
}

void LocalMediaResolverTests::matchesAlternativeTitle() {
    auto media = CatalogMedia(30, QStringLiteral("Kusuriya no Hitorigoto"));
    media.AlternativeNames = {QStringLiteral("The Apothecary Diaries")};

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("The Apothecary Diaries")), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 30);
}

void LocalMediaResolverTests::matchesPersistedSynonym() {
    auto media = CatalogMedia(40, QStringLiteral("Boku no Hero Academia"));
    media.AlternativeNames = {
        QStringLiteral("My Hero Academia"),
        QStringLiteral("Academia de Herois")
    };

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Academia de Herois")), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 40);
}

void LocalMediaResolverTests::normalizesCaseWhitespaceAndEquivalentSeparators() {
    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("  STEINS__GATE - ZERO  ")),
        {CatalogMedia(50, QStringLiteral("Steins.Gate: Zero"))});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 50);
}

void LocalMediaResolverTests::returnsUnrecognizedForUnknownTitle() {
    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Unknown Anime")),
        {CatalogMedia(60, QStringLiteral("Known Anime"))});

    QCOMPARE(match.state, LocalRecognitionState::Unrecognized);
    QCOMPARE(match.mediaId, 0);
    QVERIFY(!match.diagnostic.isEmpty());
}

void LocalMediaResolverTests::doesNotUseFuzzyMatching() {
    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Known Anime Extra")),
        {CatalogMedia(70, QStringLiteral("Known Anime"))});

    QCOMPARE(match.state, LocalRecognitionState::Unrecognized);
    QCOMPARE(match.mediaId, 0);
}

void LocalMediaResolverTests::returnsAmbiguousForMultipleCatalogMatches() {
    auto first = CatalogMedia(80, QStringLiteral("Shared Title"));
    auto second = CatalogMedia(81, QStringLiteral("Different Title"));
    second.AlternativeNames = {QStringLiteral("Shared_Title")};

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("shared-title")), {first, second});

    QCOMPARE(match.state, LocalRecognitionState::Ambiguous);
    QCOMPARE(match.mediaId, 0);
    QVERIFY(!match.diagnostic.isEmpty());
}

void LocalMediaResolverTests::doesNotTreatMultipleVariantsOfOneMediaAsAmbiguous() {
    auto media = CatalogMedia(90, QStringLiteral("Cowboy Bebop"));
    media.EnglishName = QStringLiteral("Cowboy-Bebop");
    media.AlternativeNames = {QStringLiteral("cowboy_bebop")};

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("COWBOY.BEBOP")), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 90);
}

void LocalMediaResolverTests::rejectsUnsupportedKind() {
    auto recognition = Recognized(QStringLiteral("Frieren"));
    recognition.mediaKind = LocalMediaKind::Manga;

    const auto match = LocalMediaResolver().resolve(
        recognition, {CatalogMedia(100, QStringLiteral("Frieren"))});

    QCOMPARE(match.state, LocalRecognitionState::Unsupported);
    QCOMPARE(match.mediaId, 0);
    QVERIFY(!match.diagnostic.isEmpty());
}

void LocalMediaResolverTests::rejectsInvalidEpisode_data() {
    QTest::addColumn<int>("episodeValue");
    QTest::addColumn<bool>("hasEpisode");

    QTest::newRow("missing") << 0 << false;
    QTest::newRow("zero") << 0 << true;
    QTest::newRow("negative") << -1 << true;
}

void LocalMediaResolverTests::rejectsInvalidEpisode() {
    QFETCH(int, episodeValue);
    QFETCH(bool, hasEpisode);
    const auto episode = hasEpisode ? std::optional<int>(episodeValue) : std::nullopt;

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Frieren"), episode),
        {CatalogMedia(110, QStringLiteral("Frieren"))});

    QCOMPARE(match.state, LocalRecognitionState::Ambiguous);
    QCOMPARE(match.mediaId, 0);
    QVERIFY(!match.diagnostic.isEmpty());
}

void LocalMediaResolverTests::ignoresPunctuationWhenMatchingEnglishName() {
    Media media = CatalogMedia(191656, QStringLiteral("Seitokai ni mo Ana wa Aru!"));
    media.EnglishName = QStringLiteral("Even the Student Council Has Its Holes!");

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Even the Student Council Has Its Holes")), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 191656);
}

void LocalMediaResolverTests::ignoresPunctuationWhenExtractedTitleIsDotted() {
    Media media = CatalogMedia(191656, QStringLiteral("Seitokai ni mo Ana wa Aru!"));
    media.EnglishName = QStringLiteral("Even the Student Council Has Its Holes!");

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Even.the.Student.Council.Has.Its.Holes")), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 191656);
}

void LocalMediaResolverTests::ignoresPunctuationWhenMatchingAlternativeName() {
    Media media = CatalogMedia(12, QStringLiteral("Primary"));
    media.AlternativeNames = {QStringLiteral("Mob Psycho 100: “II” (Season 2) & Co.!?")};

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Mob Psycho 100 II Season 2 Co")), {media});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 12);
}

void LocalMediaResolverTests::treatsColonAndSpaceAsEquivalent() {
    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Re:Zero kara Hajimeru Isekai Seikatsu")),
        {CatalogMedia(31, QStringLiteral("Re Zero kara Hajimeru Isekai Seikatsu"))});

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 31);
}

void LocalMediaResolverTests::returnsAmbiguousWhenPunctuationOnlyDistinguishesMedia() {
    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Title")),
        {CatalogMedia(1, QStringLiteral("Title!")), CatalogMedia(2, QStringLiteral("Title?"))});

    QCOMPARE(match.state, LocalRecognitionState::Ambiguous);
    QCOMPARE(match.mediaId, 0);
}

void LocalMediaResolverTests::dropsApostrophesInsteadOfSplittingWords() {
    const QList<Media> catalog{CatalogMedia(61, QStringLiteral("Frieren's Journey")),
                               CatalogMedia(62, QStringLiteral("JoJo’s Bizarre Adventure"))};
    for (const auto &title : {QStringLiteral("Frierens Journey"), QStringLiteral("Frieren’s Journey"),
                              QStringLiteral("Frierenʼs Journey"), QStringLiteral("Frieren＇s Journey"),
                              QStringLiteral("Frieren's Journey")}) {
        const auto match = LocalMediaResolver().resolve(Recognized(title), catalog);
        QCOMPARE(match.state, LocalRecognitionState::Associated);
        QCOMPARE(match.mediaId, 61);
    }
    const auto jojo = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("JoJos.Bizarre.Adventure")), catalog);
    QCOMPARE(jojo.state, LocalRecognitionState::Associated);
    QCOMPARE(jojo.mediaId, 62);
}

void LocalMediaResolverTests::stillMatchesNonLatinTitles() {
    const auto japanese = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("葬送のフリーレン")),
        {CatalogMedia(41, QStringLiteral("葬送のフリーレン"))});
    QCOMPARE(japanese.state, LocalRecognitionState::Associated);
    QCOMPARE(japanese.mediaId, 41);

    const auto thai = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("ซันเดอร์")),
        {CatalogMedia(42, QStringLiteral("ซันเดอร์"))});
    QCOMPARE(thai.state, LocalRecognitionState::Associated);
    QCOMPARE(thai.mediaId, 42);
}

namespace {
QList<Media> FixtureCatalog() {
    Media frieren = CatalogMedia(1, QStringLiteral("Sousou no Frieren"));
    frieren.EnglishName = QStringLiteral("Frieren: Beyond Journey's End");
    frieren.OriginalName = QStringLiteral("葬送のフリーレン");
    frieren.AlternativeNames = {QStringLiteral("Frieren at the Funeral")};
    Media jojoA = CatalogMedia(2, QStringLiteral("JoJo's Bizarre Adventure"));
    Media jojoB = CatalogMedia(3, QStringLiteral("JoJos Bizarre Adventure"));
    Media variants = CatalogMedia(4, QStringLiteral("Re:Zero"));
    variants.EnglishName = QStringLiteral("Re Zero");
    variants.AlternativeNames = {QStringLiteral("re-zero"), QStringLiteral("   ")};
    return {frieren, jojoA, jojoB, variants, CatalogMedia(5, QStringLiteral("!!!"))};
}
}

void LocalMediaResolverTests::indexedResolveMatchesCatalogResolveForEveryFixtureTitle_data() {
    QTest::addColumn<QString>("title");
    QTest::newRow("primary") << "sousou no frieren";
    QTest::newRow("english apostrophe") << "Frieren Beyond Journeys End";
    QTest::newRow("original") << "葬送のフリーレン";
    QTest::newRow("alternative") << "Frieren.at.the.Funeral";
    QTest::newRow("ambiguous after apostrophe removal") << "jojos bizarre adventure";
    QTest::newRow("one media many names") << "RE ZERO";
    QTest::newRow("punctuation only") << "!!!";
    QTest::newRow("unknown") << "Something Else";
    QTest::newRow("blank") << "   ";
}

void LocalMediaResolverTests::indexedResolveMatchesCatalogResolveForEveryFixtureTitle() {
    QFETCH(QString, title);
    const auto catalog = FixtureCatalog();
    const LocalMediaCatalogIndex index(catalog);

    const auto expected = LocalMediaResolver().resolve(Recognized(title), catalog);
    const auto actual = LocalMediaResolver().resolve(Recognized(title), index);

    QCOMPARE(actual.state, expected.state);
    QCOMPARE(actual.mediaId, expected.mediaId);
    QCOMPARE(actual.diagnostic, expected.diagnostic);
}

void LocalMediaResolverTests::indexCountsOneMediaOnceEvenWhenSeveralNamesCollapseToTheSameTitle() {
    const LocalMediaCatalogIndex index(FixtureCatalog());

    QCOMPARE(index.find(QStringLiteral("re zero")), QList<int>({4}));
    QCOMPARE(index.find(QStringLiteral("jojos bizarre adventure")), QList<int>({2, 3}));
    QVERIFY(index.find(QStringLiteral("missing")).isEmpty());
}

void LocalMediaResolverTests::indexedResolveOfLargeCatalogStaysFast() {
    QList<Media> catalog;
    for (int id = 1; id <= 2000; ++id) {
        auto media = CatalogMedia(id, QStringLiteral("Anime Title %1").arg(id));
        media.EnglishName = QStringLiteral("English Title %1").arg(id);
        media.OriginalName = QStringLiteral("Original Title %1").arg(id);
        media.AlternativeNames = {QStringLiteral("Alias %1 A").arg(id), QStringLiteral("Alias %1 B").arg(id)};
        catalog.append(media);
    }
    QElapsedTimer timer;
    timer.start();
    const LocalMediaCatalogIndex index(catalog);
    const LocalMediaResolver resolver;
    int associated = 0;
    for (int file = 0; file < 650; ++file) {
        const auto match = resolver.resolve(Recognized(QStringLiteral("anime title %1").arg(file * 3 % 2000 + 1)), index);
        if (match.state == LocalRecognitionState::Associated) ++associated;
    }

    QCOMPARE(associated, 650);
    // The quadratic implementation needs minutes for this size; a generous bound keeps this stable.
    QVERIFY2(timer.elapsed() < 10000, qPrintable(QStringLiteral("took %1 ms").arg(timer.elapsed())));
}

namespace {
LocalFileRecognition WithSeason(const QString &title, const int season, const std::optional<int> episode = 1) {
    auto recognition = Recognized(title, episode);
    recognition.season = season;
    return recognition;
}

QList<Media> KusuriyaCatalog() {
    return {CatalogMedia(99026, QStringLiteral("Kusuriya no Hitorigoto")),
            CatalogMedia(161645, QStringLiteral("Kusuriya no Hitorigoto")),
            CatalogMedia(176301, QStringLiteral("Kusuriya no Hitorigoto 2nd Season")),
            CatalogMedia(195516, QStringLiteral("Kusuriya no Hitorigoto 3rd Season"))};
}

QList<Media> KusuriyaCatalogWithFolders() {
    auto catalog = KusuriyaCatalog();
    catalog[1].LocalPath = QStringLiteral("Q:\\Animes\\Kusuriya no Hitorigoto");
    catalog[3].LocalPath = QStringLiteral("Q:\\Animes\\Kusuriya no Hitorigoto 3rd Season");
    return catalog;
}
const QString KusuriyaFile = QStringLiteral(
    "Q:\\Animes\\Kusuriya no Hitorigoto 3rd Season\\[Erai-raws] Kusuriya no Hitorigoto 3rd Season - 01 [1080p].mkv");
}

void LocalMediaResolverTests::folderAssociationWinsOverTitleMatch() {
    const LocalMediaCatalogIndex index(KusuriyaCatalogWithFolders());
    auto recognition = Recognized(QStringLiteral("Kusuriya no Hitorigoto"), 1);
    recognition.season = 3;

    const auto match = LocalMediaResolver().resolve(recognition, index, KusuriyaFile);

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 195516);
}

void LocalMediaResolverTests::folderMatchRequiresFolderBoundary() {
    Media foo = CatalogMedia(1, QStringLiteral("Foo"));
    foo.LocalPath = QStringLiteral("Q:/Animes/Foo");
    const LocalMediaCatalogIndex index({foo, CatalogMedia(2, QStringLiteral("Other"))});

    const auto sibling = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Other")), index, QStringLiteral("Q:/Animes/Foo 2/Other - 01.mkv"));
    const auto inside = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Other")), index, QStringLiteral("Q:/Animes/Foo/Other - 01.mkv"));

    QCOMPARE(sibling.mediaId, 2);
    QCOMPARE(inside.mediaId, 1);
}

void LocalMediaResolverTests::deepestFolderWins() {
    Media parent = CatalogMedia(1, QStringLiteral("Parent"));
    parent.LocalPath = QStringLiteral("Q:/Animes");
    Media child = CatalogMedia(2, QStringLiteral("Child"));
    child.LocalPath = QStringLiteral("Q:/Animes/Child");
    const LocalMediaCatalogIndex index({parent, child});

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Parent")), index, QStringLiteral("Q:/Animes/Child/Season 1/Parent - 01.mkv"));

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 2);
}

void LocalMediaResolverTests::folderMatchIgnoresSeparatorStyleAndCaseOnWindows() {
#ifdef Q_OS_WIN
    Media media = CatalogMedia(7, QStringLiteral("Anything"));
    media.LocalPath = QStringLiteral("q:\\ANIMES\\Show\\");
    const LocalMediaCatalogIndex index({media});

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Unrelated")), index, QStringLiteral("Q:/Animes/show/Episode - 01.mkv"));

    QCOMPARE(match.mediaId, 7);
#else
    QSKIP("Case-insensitive folder comparison applies to Windows only.");
#endif
}

void LocalMediaResolverTests::folderMatchWithoutValidEpisodeIsAmbiguous() {
    Media media = CatalogMedia(7, QStringLiteral("Show"));
    media.LocalPath = QStringLiteral("Q:/Animes/Show");
    const LocalMediaCatalogIndex index({media});

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Show"), std::nullopt), index, QStringLiteral("Q:/Animes/Show/extras.mkv"));

    QCOMPARE(match.state, LocalRecognitionState::Ambiguous);
    QVERIFY(!match.diagnostic.isEmpty());
}

void LocalMediaResolverTests::unrelatedFolderFallsBackToTitleMatching() {
    Media show = CatalogMedia(7, QStringLiteral("Show"));
    show.LocalPath = QStringLiteral("Q:/Animes/Show");
    const LocalMediaCatalogIndex index({show, CatalogMedia(8, QStringLiteral("Other"))});

    const auto match = LocalMediaResolver().resolve(
        Recognized(QStringLiteral("Other")), index, QStringLiteral("Q:/Animes/Elsewhere/Other - 01.mkv"));

    QCOMPARE(match.state, LocalRecognitionState::Associated);
    QCOMPARE(match.mediaId, 8);
}

QTEST_MAIN(LocalMediaResolverTests)
#include "LocalMediaResolverTests.moc"
