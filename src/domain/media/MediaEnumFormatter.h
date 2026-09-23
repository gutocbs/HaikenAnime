#ifndef HAIKENANIME_MEDIAENUMFORMATTER_H
#define HAIKENANIME_MEDIAENUMFORMATTER_H

#include <QString>

#include "MediaStatus.h"
#include "MediaType.h"

/** Converts media enums into stable human-readable text for diagnostics and presentation. */
class MediaEnumFormatter final {
public:
    /** Returns the display name for a media type. */
    [[nodiscard]] static QString TypeToString(MediaType type);

    /** Returns the display name for a media status. */
    [[nodiscard]] static QString StatusToString(MediaStatus status);
};

#endif // HAIKENANIME_MEDIAENUMFORMATTER_H
