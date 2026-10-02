#ifndef HAIKENANIME_COVERSOURCERESOLVER_H
#define HAIKENANIME_COVERSOURCERESOLVER_H

#include "CoverQuality.h"
#include "../../domain/media/Media.h"

[[nodiscard]] QString ResolveCoverSource(const Media &media, CoverQuality requestedQuality);

#endif
