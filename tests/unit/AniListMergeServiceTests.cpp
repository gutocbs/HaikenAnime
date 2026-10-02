#include <QtTest>

#include "../../src/application/anilist/AniListMergeService.h"

class AniListMergeServiceTests : public QObject {
    Q_OBJECT

private slots:
    void remoteWinsForCatalogField();
    void maxValueKeepsHighestProgress();
    void localWinsLatestKeepsLocalValue();
    void localStatusIsQueuedForUpdate();
    void deletionRequiresConfirmation();
    void incompatibleProgressValuesProduceConflict();
};

void AniListMergeServiceTests::remoteWinsForCatalogField() {
    const auto decision = AniListMergeService::Merge(
        AniListField::Title, QStringLiteral("Local"), QStringLiteral("Remote"));
    QCOMPARE(std::get<QString>(decision.value), QStringLiteral("Remote"));
    QCOMPARE(decision.result, AniListMergeResult::Applied);
}

void AniListMergeServiceTests::maxValueKeepsHighestProgress() {
    const auto decision = AniListMergeService::Merge(AniListField::Progress, 12, 10);
    QCOMPARE(std::get<int>(decision.value), 12);
}

void AniListMergeServiceTests::localWinsLatestKeepsLocalValue() {
    const auto decision = AniListMergeService::Merge(
        AniListField::ListStatus, QStringLiteral("WATCHING"), QStringLiteral("COMPLETED"));
    QCOMPARE(std::get<QString>(decision.value), QStringLiteral("WATCHING"));
    QCOMPARE(decision.policy, AniListMergePolicy::LocalWinsLatest);
    QCOMPARE(decision.result, AniListMergeResult::Applied);
}

void AniListMergeServiceTests::localStatusIsQueuedForUpdate() {
    const auto decision = AniListMergeService::Merge(
        AniListField::PersonalScore, 9, 8);
    QCOMPARE(std::get<int>(decision.value), 9);
    QCOMPARE(decision.result, AniListMergeResult::Queued);
}

void AniListMergeServiceTests::deletionRequiresConfirmation() {
    const auto decision = AniListMergeService::Merge(
        AniListField::Deletion, QStringLiteral("keep"), QStringLiteral("delete"));
    QCOMPARE(decision.result, AniListMergeResult::RequiresConfirmation);
}

void AniListMergeServiceTests::incompatibleProgressValuesProduceConflict() {
    const auto decision = AniListMergeService::Merge(
        AniListField::Progress, QStringLiteral("twelve"), 13);
    QCOMPARE(decision.result, AniListMergeResult::Conflict);
}

QTEST_MAIN(AniListMergeServiceTests)
#include "AniListMergeServiceTests.moc"
