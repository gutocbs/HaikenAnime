#include "AniListMergePolicyResolver.h"

AniListMergePolicy AniListMergePolicyResolver::PolicyFor(const AniListField field) {
    switch (field) {
    case AniListField::ExternalId:
    case AniListField::Title:
    case AniListField::AlternativeTitles:
    case AniListField::CoverUrl:
    case AniListField::Synopsis:
        return AniListMergePolicy::RemoteWins;
    case AniListField::Progress:
        return AniListMergePolicy::MaxValue;
    case AniListField::PersonalScore:
        return AniListMergePolicy::QueueLocalChange;
    case AniListField::ListStatus:
    case AniListField::LocalFiles:
        return AniListMergePolicy::LocalWins;
    case AniListField::Deletion:
        return AniListMergePolicy::RequiresConfirmation;
    }

    return AniListMergePolicy::RequiresConfirmation;
}
