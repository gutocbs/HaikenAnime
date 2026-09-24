#ifndef HAIKENANIME_ANILISTPENDINGCHANGEFACTORY_H
#define HAIKENANIME_ANILISTPENDINGCHANGEFACTORY_H

#include <QString>

#include "AniListPendingChange.h"

/** Creates validated pending changes from local field transitions. */
class AniListPendingChangeFactory final {
public:
    /** Creates a writable, typed transition; deletions start awaiting confirmation. */
    [[nodiscard]] static bool Create(int mediaId, AniListField field,
                                     const AniListFieldValue &previousValue,
                                     const AniListFieldValue &newValue,
                                     AniListPendingChange &change, QString &error);
};

#endif // HAIKENANIME_ANILISTPENDINGCHANGEFACTORY_H
