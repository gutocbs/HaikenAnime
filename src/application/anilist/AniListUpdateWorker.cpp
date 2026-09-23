#include "AniListUpdateWorker.h"

AniListUpdateWorker::AniListUpdateWorker(AniListPendingChangeProcessor &processor,
                                         QObject *parent)
    : QObject(parent), processor_(processor) {
}

void AniListUpdateWorker::Run(const int mediaId) {
    emit Started();
    QString error;
    if (!processor_.Process(mediaId, error)) {
        emit Failed(error);
        return;
    }
    emit Completed();
}
