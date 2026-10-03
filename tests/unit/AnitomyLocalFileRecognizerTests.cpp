#include <QtTest>

#include "../../src/infrastructure/library/AnitomyLocalFileRecognizer.h"

class AnitomyLocalFileRecognizerTests final : public QObject {
    Q_OBJECT

private slots:
    void recognizesTitleEpisodeAndSeason();
    void rejectsMissingTitleOrEpisode();
    void rejectsRangesAndMultipleEpisodes();
    void rejectsUnsupportedKinds();
};

void AnitomyLocalFileRecognizerTests::recognizesTitleEpisodeAndSeason() {
    AnitomyLocalFileRecognizer recognizer;
    const auto result = recognizer.recognize(QStringLiteral("[Group] Frieren - 01 [1080p].mkv"));
    QCOMPARE(result.state, LocalRecognitionState::Recognized);
    QCOMPARE(result.mediaKind, LocalMediaKind::Anime);
    QCOMPARE(result.extractedTitle, QStringLiteral("Frieren"));
    QCOMPARE(result.episode, std::optional<int>(1));
    QVERIFY(!result.season.has_value());
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
