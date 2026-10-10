#include "AniListPendingChangeDrain.h"

AniListPendingChangeDrain::AniListPendingChangeDrain(
    IPendingChangeRepository &repository, AniListPendingChangeProcessor &processor)
    : repository_(repository), processor_(processor) {
}

bool AniListPendingChangeDrain::processAll(QString &error,
                                           const CancellationProbe &isCancelled) {
    error.clear();
    while (true) {
        if (isCancelled && isCancelled()) {
            error = QStringLiteral("AniList pending-change processing cancelled.");
            return false;
        }

        QList<int> mediaIds;
        if (!repository_.getPendingMediaIds(mediaIds, error)) return false;
        if (mediaIds.isEmpty()) return true;

        for (const int mediaId : mediaIds) {
            if (isCancelled && isCancelled()) {
                error = QStringLiteral("AniList pending-change processing cancelled.");
                return false;
            }
            if (!processor_.process(mediaId, error)) return false;
        }
    }
}
