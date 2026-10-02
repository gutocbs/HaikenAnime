#ifndef HAIKENANIME_IPERSONALLISTMEDIAWRITER_H
#define HAIKENANIME_IPERSONALLISTMEDIAWRITER_H

#include <QString>

#include "../../domain/media/Media.h"

/** Updates only user-owned fields of an existing local personal-list record. */
class IPersonalListMediaWriter {
public:
    virtual ~IPersonalListMediaWriter() = default;
    [[nodiscard]] virtual bool updatePersonalListMedia(const Media &media, QString &error) = 0;
};

#endif // HAIKENANIME_IPERSONALLISTMEDIAWRITER_H
