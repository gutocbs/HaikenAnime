#include <QtTest>

#include "../../src/application/anilist/AniListMergePolicyResolver.h"

class AniListMergePolicyResolverTests : public QObject {
    Q_OBJECT

private slots:
    void mapsCatalogFieldsToRemoteWins();
    void mapsUserFieldsToExpectedPolicies();
};

void AniListMergePolicyResolverTests::mapsCatalogFieldsToRemoteWins() {
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::Title),
             AniListMergePolicy::RemoteWins);
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::CoverUrl),
             AniListMergePolicy::RemoteWins);
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::Synopsis),
             AniListMergePolicy::RemoteWins);
}

void AniListMergePolicyResolverTests::mapsUserFieldsToExpectedPolicies() {
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::Progress),
             AniListMergePolicy::MaxValue);
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::ListStatus),
             AniListMergePolicy::LocalWins);
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::PersonalScore),
             AniListMergePolicy::QueueLocalChange);
    QCOMPARE(AniListMergePolicyResolver::PolicyFor(AniListField::Deletion),
             AniListMergePolicy::RequiresConfirmation);
}

QTEST_MAIN(AniListMergePolicyResolverTests)
#include "AniListMergePolicyResolverTests.moc"
