#ifndef HAIKENANIME_IPERSONALLISTCHANGEWRITER_H
#define HAIKENANIME_IPERSONALLISTCHANGEWRITER_H

#include <QList>
#include <QString>

#include "../../domain/media/Media.h"
#include "../anilist/AniListPendingChange.h"

/** Persists one user edit and its remote outbox entries as one local operation. */
class IPersonalListChangeWriter {
public:
    virtual ~IPersonalListChangeWriter() = default;

    /** Hides the media from the personal list and queues its AniList deletion in one operation. */
    [[nodiscard]] virtual bool remove(const Media &media, const AniListPendingChange &deletion,
                                      QString &error) = 0;

    [[nodiscard]] virtual bool save(const Media &media, const QList<AniListPendingChange> &changes,
                                    QString &error) = 0;
};

#endif // HAIKENANIME_IPERSONALLISTCHANGEWRITER_H
