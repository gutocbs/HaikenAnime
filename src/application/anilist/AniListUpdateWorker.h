#ifndef HAIKENANIME_ANILISTUPDATEWORKER_H
#define HAIKENANIME_ANILISTUPDATEWORKER_H

#include <QObject>

#include "AniListPendingChangeProcessor.h"

/** Runs pending AniList mutations on a dedicated worker thread. */
class AniListUpdateWorker final : public QObject {
    Q_OBJECT

public:
    /** Creates a worker that delegates pending changes to processor. */
    explicit AniListUpdateWorker(AniListPendingChangeProcessor &processor,
                                 QObject *parent = nullptr);

public slots:
    /** Processes pending changes for one media item. */
    void Run(int mediaId);

signals:
    void Started();
    void Completed();
    void Failed(const QString &error);

private:
    AniListPendingChangeProcessor &processor_;
};

#endif // HAIKENANIME_ANILISTUPDATEWORKER_H
