#include <QtTest>

#include "../../src/application/recognition/ProgressDecisionService.h"

namespace {
class FakeMediaProgressReader final : public IMediaProgressReader {
public:
    bool find(const int mediaId, std::optional<Media> &media, QString &error) override {
        ++lookups;
        error.clear();
        if (fail) {
            error = QStringLiteral("database locked");
            return false;
        }
        media = library.contains(mediaId) ? std::optional<Media>(library.value(mediaId)) : std::nullopt;
        return true;
    }

    QHash<int, Media> library;
    bool fail = false;
    int lookups = 0;
};

Media onePiece(const int consumed, const int total = 12, const UserListStatus status = UserListStatus::Current) {
    Media media;
    media.Id = 21;
    media.Name = QStringLiteral("One Piece");
    media.ConsumedChapters = consumed;
    media.TotalChapters = total;
    media.ListStatus = status;
    return media;
}

EpisodeCandidate candidate(const int episode, const std::optional<int> mediaId = 21, const double confidence = 1.0) {
    return EpisodeCandidate::create(QStringLiteral("One Piece"), mediaId, 1, episode, confidence).value();
}
}

class ProgressDecisionServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void init();
    void lowConfidenceIsUncertainWithoutLookup();
    void missingMediaIdIsUncertain();
    void unknownMediaIsIgnored();
    void lookupFailureIsIgnoredWithReason();
    void watchedEpisodeIsNeverRegressed();
    void nextEpisodeMarksWatched();
    void laterUnseenEpisodeAdvancesProgress();
    void finalEpisodeCompletesTheList();
    void planningMediaMovesToCurrent();
    void episodeBeyondKnownTotalIsUncertain();
    void unknownTotalAcceptsAnyUnseenEpisode();

private:
    FakeMediaProgressReader reader;
};

void ProgressDecisionServiceTests::init() {
    reader = FakeMediaProgressReader();
}

void ProgressDecisionServiceTests::lowConfidenceIsUncertainWithoutLookup() {
    reader.library.insert(21, onePiece(0));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(1, 21, 0.5));

    QCOMPARE(decision.kind, ProgressDecisionKind::Uncertain);
    QCOMPARE(reader.lookups, 0);
}

void ProgressDecisionServiceTests::missingMediaIdIsUncertain() {
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(1, std::nullopt));

    QCOMPARE(decision.kind, ProgressDecisionKind::Uncertain);
    QVERIFY(!decision.updated.has_value());
}

void ProgressDecisionServiceTests::unknownMediaIsIgnored() {
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(1));

    QCOMPARE(decision.kind, ProgressDecisionKind::Ignored);
}

void ProgressDecisionServiceTests::lookupFailureIsIgnoredWithReason() {
    reader.fail = true;
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(1));

    QCOMPARE(decision.kind, ProgressDecisionKind::Ignored);
    QVERIFY(!decision.reason.isEmpty());
}

void ProgressDecisionServiceTests::watchedEpisodeIsNeverRegressed() {
    reader.library.insert(21, onePiece(5));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    for (const int episode : {3, 5}) {
        const auto decision = service.evaluate(candidate(episode));
        QCOMPARE(decision.kind, ProgressDecisionKind::AlreadyWatched);
        QVERIFY(!decision.updated.has_value());
    }
}

void ProgressDecisionServiceTests::nextEpisodeMarksWatched() {
    reader.library.insert(21, onePiece(5));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(6));

    QCOMPARE(decision.kind, ProgressDecisionKind::MarkWatched);
    QVERIFY(decision.previous.has_value());
    QVERIFY(decision.updated.has_value());
    QCOMPARE(decision.previous->ConsumedChapters, 5);
    QCOMPARE(decision.updated->ConsumedChapters, 6);
    QCOMPARE(decision.updated->ListStatus, UserListStatus::Current);
}

void ProgressDecisionServiceTests::laterUnseenEpisodeAdvancesProgress() {
    reader.library.insert(21, onePiece(2));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(5));

    QCOMPARE(decision.kind, ProgressDecisionKind::MarkWatched);
    QCOMPARE(decision.updated->ConsumedChapters, 5);
}

void ProgressDecisionServiceTests::finalEpisodeCompletesTheList() {
    reader.library.insert(21, onePiece(11, 12));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(12));

    QCOMPARE(decision.kind, ProgressDecisionKind::MarkWatched);
    QCOMPARE(decision.updated->ListStatus, UserListStatus::Completed);
}

void ProgressDecisionServiceTests::planningMediaMovesToCurrent() {
    reader.library.insert(21, onePiece(0, 12, UserListStatus::Planning));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(1));

    QCOMPARE(decision.kind, ProgressDecisionKind::MarkWatched);
    QCOMPARE(decision.updated->ListStatus, UserListStatus::Current);
}

void ProgressDecisionServiceTests::episodeBeyondKnownTotalIsUncertain() {
    reader.library.insert(21, onePiece(3, 12));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(13));

    QCOMPARE(decision.kind, ProgressDecisionKind::Uncertain);
}

void ProgressDecisionServiceTests::unknownTotalAcceptsAnyUnseenEpisode() {
    reader.library.insert(21, onePiece(1000, 0));
    const ProgressDecisionService service(reader, RecognitionPolicy());

    const auto decision = service.evaluate(candidate(1001));

    QCOMPARE(decision.kind, ProgressDecisionKind::MarkWatched);
    QCOMPARE(decision.updated->ListStatus, UserListStatus::Current);
}

QTEST_MAIN(ProgressDecisionServiceTests)
#include "ProgressDecisionServiceTests.moc"
