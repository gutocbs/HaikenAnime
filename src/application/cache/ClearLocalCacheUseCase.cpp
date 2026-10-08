#include "ClearLocalCacheUseCase.h"

#include <QtGlobal>

#include <utility>

ClearLocalCacheUseCase::ClearLocalCacheUseCase(
    CoverDownloadCoordinator &covers, const ICoverTemporaryStore &temporaryFiles,
    QList<ICacheCleanupParticipant *> participants)
    : covers_(covers), temporaryFiles_(temporaryFiles), participants_(std::move(participants))
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
    QString temporaryError;
    if (!temporaryFiles_.ClearAbandoned(result.removedCoverTemporaryFiles, temporaryError)) {
        result.failures.append({QStringLiteral("cover-temporary-files"), temporaryError});
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

    auto finalState = ClearLocalCacheState::Running;
    if (state_.compare_exchange_strong(finalState, ClearLocalCacheState::Idle)) {
        result.cancelled = false;
    } else {
        result.cancelled = finalState == ClearLocalCacheState::CancellationRequested;
        state_.store(ClearLocalCacheState::Idle);
    }
    if (completion) completion(std::move(result));
    return true;
}

bool ClearLocalCacheUseCase::Cancel()
{
    auto expected = ClearLocalCacheState::Running;
    return state_.compare_exchange_strong(expected, ClearLocalCacheState::CancellationRequested);
}

ClearLocalCacheState ClearLocalCacheUseCase::State() const
{
    return state_.load();
}
