#ifndef HAIKENANIME_IMEDIASNAPSHOTRECONCILER_H
#define HAIKENANIME_IMEDIASNAPSHOTRECONCILER_H

#include <QSet>
#include <QString>

class IMediaSnapshotReconciler {
public:
    virtual ~IMediaSnapshotReconciler() = default;

    [[nodiscard]] virtual bool reconcileAuthoritativeSnapshot(
        const QSet<int> &observedMediaIds, int &removedCount, QString &error) = 0;
};

#endif
