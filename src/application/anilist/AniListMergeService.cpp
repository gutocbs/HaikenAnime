#include "AniListMergeService.h"

#include <algorithm>

AniListMergeDecision AniListMergeService::Merge(
    const AniListField field, const AniListFieldValue &localValue,
    const AniListFieldValue &remoteValue) {
    const auto policy = AniListMergePolicyResolver::PolicyFor(field);
    AniListMergeDecision decision{remoteValue, policy, AniListMergeResult::Applied};

    switch (policy) {
    case AniListMergePolicy::RemoteWins:
        decision.value = remoteValue;
        break;
    case AniListMergePolicy::LocalWins:
        decision.value = localValue;
        break;
    case AniListMergePolicy::MaxValue:
        if (std::holds_alternative<int>(localValue) && std::holds_alternative<int>(remoteValue)) {
            decision.value = std::max(std::get<int>(localValue), std::get<int>(remoteValue));
        } else {
            decision.value = localValue;
        }
        break;
    case AniListMergePolicy::QueueLocalChange:
        decision.value = localValue;
        decision.result = localValue == remoteValue ? AniListMergeResult::Ignored
                                                     : AniListMergeResult::Queued;
        break;
    case AniListMergePolicy::RequiresConfirmation:
        decision.value = localValue;
        decision.result = AniListMergeResult::RequiresConfirmation;
        break;
    }

    return decision;
}
