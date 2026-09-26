#ifndef HAIKENANIME_ILOCALFILEREPOSITORY_H
#define HAIKENANIME_ILOCALFILEREPOSITORY_H

#include <QList>
#include "../../domain/library/LocalFileRecord.h"

class ILocalFileRepository {
public:
    virtual ~ILocalFileRepository() = default;

    // Creates a running scan; returns its ID on success, or zero and an error on failure.
    virtual bool beginScan(const QString &rootPath, qint64 &scanId, QString &error) = 0;
    // Atomically persists one batch for a running scan. Paths must already be normalized
    // by the scanner; identity is rootPath plus normalizedRelativePath.
    virtual bool upsertBatch(qint64 scanId, const QList<LocalFileObservation> &observations, QString &error) = 0;
    // Atomically reconciles unseen files for this root and marks the scan succeeded.
    virtual bool completeScan(qint64 scanId, qsizetype observedCount, QString &error) = 0;
    // Accepts only Failed or Interrupted; does not reconcile unavailable files.
    virtual bool failScan(qint64 scanId, LibraryScanStatus status, qsizetype observedCount,
                          const QString &diagnostic, QString &error) = 0;
};

#endif
