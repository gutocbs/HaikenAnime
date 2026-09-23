#include "AniListSyncWorker.h"

AniListSyncWorker::AniListSyncWorker(AniListSyncService &service, QObject *parent)
    : QObject(parent), service_(service) {
}

void AniListSyncWorker::Run(const AniListSyncFilter &filter) {
    emit Started();
    QString error;
    if (!service_.Synchronize(filter, error)) {
        emit Failed(error);
        return;
    }
    emit Completed();
}
