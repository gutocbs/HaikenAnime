#include "ClearLocalCacheUseCase.h"

#include <utility>

ClearLocalCacheUseCase::ClearLocalCacheUseCase(CoverDownloadCoordinator &covers,
                                               const ICoverTemporaryStore &temporaryFiles)
    : covers_(covers), temporaryFiles_(temporaryFiles)
{
}

bool ClearLocalCacheUseCase::Start(Completion completion)
{
    if (running_) return false;
    running_ = true;

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

    running_ = false;
    if (completion) completion(std::move(result));
    return true;
}
