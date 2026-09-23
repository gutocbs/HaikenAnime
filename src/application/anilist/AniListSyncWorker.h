#ifndef HAIKENANIME_ANILISTSYNCWORKER_H
#define HAIKENANIME_ANILISTSYNCWORKER_H

#include <QObject>

#include "AniListSyncService.h"

/** Runs AniList synchronization on the thread that owns this worker. */
class AniListSyncWorker final : public QObject {
    Q_OBJECT

public:
    /** Creates a worker that delegates synchronization to service. */
    explicit AniListSyncWorker(AniListSyncService &service, QObject *parent = nullptr);

public slots:
    /** Runs one synchronization and emits the corresponding lifecycle signal. */
    void Run(const AniListSyncFilter &filter);

signals:
    void Started();
    void Completed();
    void Failed(const QString &error);

private:
    AniListSyncService &service_;
};

#endif // HAIKENANIME_ANILISTSYNCWORKER_H
