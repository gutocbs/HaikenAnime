#include "AniListSyncWorker.h"

AniListSyncWorker::AniListSyncWorker(AniListSyncService &service, QObject *parent)
    : QObject(parent), service_(service) {
}

void AniListSyncWorker::Run(const MediaSyncFilter &filter) {
    emit Started();
    QString error;
    if (!service_.synchronize(filter, error)) {
        emit Failed(error);
        return;
    }
    emit Completed();
}
