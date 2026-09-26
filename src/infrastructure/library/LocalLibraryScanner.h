#ifndef HAIKENANIME_LOCALLIBRARYSCANNER_H
#define HAIKENANIME_LOCALLIBRARYSCANNER_H

#include "../../application/library/ILocalLibraryScanner.h"
#include "IDirectoryEnumerator.h"

class LocalLibraryScanner final : public ILocalLibraryScanner {
public:
    explicit LocalLibraryScanner(IDirectoryEnumerator &enumerator);
    LocalLibraryScanResult scan(const LocalLibraryScanRequest &request,
        const std::function<bool(const QList<LocalFileObservation> &, QString &)> &batchConsumer,
        const std::function<void(const LocalLibraryScanProgress &)> &progress,
        const std::function<bool()> &stopRequested) override;
private:
    IDirectoryEnumerator &enumerator_;
};

#endif
