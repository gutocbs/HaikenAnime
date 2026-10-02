#ifndef HAIKENANIME_ILOCALLIBRARYSCANNER_H
#define HAIKENANIME_ILOCALLIBRARYSCANNER_H

#include <functional>
#include <QList>
#include "LocalLibraryScanTypes.h"
#include "../../domain/library/LocalFileRecord.h"

class ILocalLibraryScanner {
public:
    virtual ~ILocalLibraryScanner() = default;
    // Runs synchronously on the caller's thread. The consumer accepts bounded metadata
    // batches or returns false with an error. Progress/stop callbacks are optional.
    // Failed/interrupted scans retain delivered batches and must not reconcile inventory.
    virtual LocalLibraryScanResult scan(const LocalLibraryScanRequest &request,
        const std::function<bool(const QList<LocalFileObservation> &, QString &)> &batchConsumer,
        const std::function<void(const LocalLibraryScanProgress &)> &progress,
        const std::function<bool()> &stopRequested) = 0;
};

#endif
