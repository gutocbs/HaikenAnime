#include "CoverSourceResolver.h"

QString ResolveCoverSource(const Media &media, const CoverQuality requestedQuality) {
    QString requested;
    QStringList alternatives;
    switch (requestedQuality) {
    case CoverQuality::Medium:
        requested = media.CoverMediumUrl;
        alternatives = {media.CoverLargeUrl, media.CoverExtraLargeUrl};
        break;
    case CoverQuality::Large:
        requested = media.CoverLargeUrl;
        alternatives = {media.CoverMediumUrl, media.CoverExtraLargeUrl};
        break;
    case CoverQuality::ExtraLarge:
        requested = media.CoverExtraLargeUrl;
        alternatives = {media.CoverLargeUrl, media.CoverMediumUrl};
        break;
    }
    if (!requested.isEmpty()) return requested;
    if (!media.CoverUrl.isEmpty()) return media.CoverUrl;
    for (const auto &alternative : alternatives) {
        if (!alternative.isEmpty()) return alternative;
    }
    return {};
}
