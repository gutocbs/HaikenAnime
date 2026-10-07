#include "ClearLocalCacheUseCase.h"

#include <QtGlobal>

#include <utility>

ClearLocalCacheUseCase::ClearLocalCacheUseCase(
    CoverDownloadCoordinator &covers, QList<ICacheCleanupParticipant *> participants)
    : covers_(covers), participants_(std::move(participants))
{
}

bool ClearLocalCacheUseCase::Start(Completion completion)
{
    auto expected = ClearLocalCacheState::Idle;
    if (!state_.compare_exchange_strong(expected, ClearLocalCacheState::Running)) return false;

    ClearLocalCacheResult result;
    const auto coverResult = covers_.Clear();
    result.removedCoverFiles = coverResult.removedFiles;
    result.removedCoverEntries = coverResult.removedEntries;
    for (const auto &failure : coverResult.failures) {
        result.failures.append({failure.component, failure.error});
    }

    const CacheCleanupCancellationProbe isCancelled = [this] {
        return state_.load() == ClearLocalCacheState::CancellationRequested;
    };
    for (auto *participant : std::as_const(participants_)) {
        if (isCancelled()) break;
        if (participant == nullptr) {
            result.failures.append({QStringLiteral("derived-data"),
                                    QStringLiteral("A registered cleanup participant is unavailable.")});
            continue;
        }
        const auto participantResult = participant->Clear(isCancelled);
        result.removedDerivedItems += qMax(0, participantResult.removedItems);
        if (!participantResult.error.isEmpty()) {
            result.failures.append({participant->Name(), participantResult.error});
        }
    }

    result.cancelled = isCancelled();
    state_.store(ClearLocalCacheState::Idle);
    if (completion) completion(std::move(result));
    return true;
}

void ClearLocalCacheUseCase::Cancel()
{
    auto expected = ClearLocalCacheState::Running;
    state_.compare_exchange_strong(expected, ClearLocalCacheState::CancellationRequested);
}

ClearLocalCacheState ClearLocalCacheUseCase::State() const
{
    return state_.load();
}
