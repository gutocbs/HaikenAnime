#include <QtTest>

#include "../../src/infrastructure/library/AnitomyLocalFileRecognizer.h"

class AnitomyLocalFileRecognizerTests final : public QObject {
    Q_OBJECT

private slots:
    void recognizesTitleEpisodeAndSeason();
    void recognizesDottedReleaseNameWithPunctuationFreeTitle();
    void rejectsMissingTitleOrEpisode();
    void rejectsRangesAndMultipleEpisodes();
    void rejectsUnsupportedKinds();
    void recoversSeasonAndEpisodeWhenAnitomyMissesThem();
    void recoversEpisodeWithVersionSuffix();
    void recoversSeasonAndEpisodeFromSxxExx();
    void recoversSeasonFromPart();
    void keepsOrdinalSeasonHandledByAnitomy();
    void doesNotTakeYearAsFallbackEpisode();
    void neverOverridesValidAnitomyEpisode();
};

void AnitomyLocalFileRecognizerTests::recoversSeasonAndEpisodeWhenAnitomyMissesThem() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral(
        "[Erai-raws] Ao no Hako Season 2 - 01 [1080p NF WEB-DL AVC EAC3][MultiSub][54AB16DB].mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.extractedTitle, QStringLiteral("Ao no Hako"));
    QCOMPARE(result.season, std::optional<int>(2));
    QCOMPARE(result.episode, std::optional<int>(1));
}

void AnitomyLocalFileRecognizerTests::recoversEpisodeWithVersionSuffix() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral(
        "[Erai-raws] Ao no Hako Season 2 - 12v2 [1080p][54AB16DB].mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.extractedTitle, QStringLiteral("Ao no Hako"));
    QCOMPARE(result.season, std::optional<int>(2));
    QCOMPARE(result.episode, std::optional<int>(12));
}

void AnitomyLocalFileRecognizerTests::recoversSeasonAndEpisodeFromSxxExx() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("Show.S02E05.1080p.mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.season, std::optional<int>(2));
    QCOMPARE(result.episode, std::optional<int>(5));
}

void AnitomyLocalFileRecognizerTests::recoversSeasonFromPart() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("Show Part 2 - 03.mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.extractedTitle, QStringLiteral("Show"));
    QCOMPARE(result.season, std::optional<int>(2));
    QCOMPARE(result.episode, std::optional<int>(3));
}

void AnitomyLocalFileRecognizerTests::keepsOrdinalSeasonHandledByAnitomy() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral(
        "[Erai-raws] Kusuriya no Hitorigoto 3rd Season - 01 [1080p].mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.extractedTitle, QStringLiteral("Kusuriya no Hitorigoto"));
    QCOMPARE(result.season, std::optional<int>(3));
    QCOMPARE(result.episode, std::optional<int>(1));
}

void AnitomyLocalFileRecognizerTests::doesNotTakeYearAsFallbackEpisode() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("Movie Title - 2024 [1080p].mkv"));
    QVERIFY(result.state != LocalRecognitionState::Recognized);
    QVERIFY(!result.episode.has_value());
}

void AnitomyLocalFileRecognizerTests::neverOverridesValidAnitomyEpisode() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("[Group] Frieren - 07 [1080p].mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.episode, std::optional<int>(7));
    QVERIFY(!result.season.has_value());
}

void AnitomyLocalFileRecognizerTests::recognizesTitleEpisodeAndSeason() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("[Group] Frieren - 01 [1080p].mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.mediaKind, LocalMediaKind::Anime);
    QCOMPARE(result.extractedTitle, QStringLiteral("Frieren"));
    QCOMPARE(result.episode, std::optional<int>(1));
    QVERIFY(!result.season.has_value());
}

void AnitomyLocalFileRecognizerTests::recognizesDottedReleaseNameWithPunctuationFreeTitle() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral(
        "Even.the.Student.Council.Has.Its.Holes.S01E01.1080p.UNCENSORED.ADN.WEB-DL.JPN.AAC2.0.H.264.MSubs-ToonsHub.mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.extractedTitle, QStringLiteral("Even the Student Council Has Its Holes"));
    QCOMPARE(result.season, std::optional<int>(1));
    QCOMPARE(result.episode, std::optional<int>(1));
}

void AnitomyLocalFileRecognizerTests::rejectsMissingTitleOrEpisode() {
    AnitomyLocalFileRecognizer recognizer;
    const auto missingEpisode = recognizer.recognize(QStringLiteral("[Group] Frieren [1080p].mkv"));
    QCOMPARE(missingEpisode.state, LocalRecognitionState::Unrecognized);
    QVERIFY(!missingEpisode.diagnostic.isEmpty());

    const auto missingTitle = recognizer.recognize(QStringLiteral("[Group] - 01 [1080p].mkv"));
    QCOMPARE(missingTitle.state, LocalRecognitionState::Unrecognized);
    QVERIFY(!missingTitle.diagnostic.isEmpty());
}

void AnitomyLocalFileRecognizerTests::rejectsRangesAndMultipleEpisodes() {
    AnitomyLocalFileRecognizer recognizer;
    const auto range = recognizer.recognize(QStringLiteral("[Group] Frieren - 01-02 [1080p].mkv"));
    QCOMPARE(range.state, LocalRecognitionState::Ambiguous);
    QVERIFY(!range.episode.has_value());

    const auto multiple = recognizer.recognize(QStringLiteral("[Group] Frieren - 01, 02 [1080p].mkv"));
    QCOMPARE(multiple.state, LocalRecognitionState::Ambiguous);
    QVERIFY(!multiple.episode.has_value());
}

void AnitomyLocalFileRecognizerTests::rejectsUnsupportedKinds() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("Frieren - 01.mkv"), LocalMediaKind::Manga);
    QCOMPARE(result.state, LocalRecognitionState::Unsupported);
    QCOMPARE(result.mediaKind, LocalMediaKind::Manga);
}

QTEST_MAIN(AnitomyLocalFileRecognizerTests)
#include "AnitomyLocalFileRecognizerTests.moc"
