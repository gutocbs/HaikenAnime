#ifndef HAIKENANIME_ANILISTSYNCERRORCLASSIFIER_H
#define HAIKENANIME_ANILISTSYNCERRORCLASSIFIER_H

#include "AniListSyncErrorCategory.h"
#include <QString>

/** Maps current infrastructure error messages to synchronization categories. */
class AniListSyncErrorClassifier final {
public:
    [[nodiscard]] static AniListSyncErrorCategory Classify(const QString &error);
};

#endif // HAIKENANIME_ANILISTSYNCERRORCLASSIFIER_H
