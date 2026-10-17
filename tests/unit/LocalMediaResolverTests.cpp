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
    void stillMatchesNonLatinTitles();
    void rejectsUnsupportedKind();
    void rejectsInvalidEpisode_data();
    void rejectsInvalidEpisode();
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

QTEST_MAIN(LocalMediaResolverTests)
#include "LocalMediaResolverTests.moc"
