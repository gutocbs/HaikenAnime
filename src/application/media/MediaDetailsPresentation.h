#ifndef HAIKENANIME_MEDIADETAILSPRESENTATION_H
#define HAIKENANIME_MEDIADETAILSPRESENTATION_H

#include <QString>
#include <QVariantList>

#include "../../domain/media/Media.h"

[[nodiscard]] QString PresentMediaSeason(const Media &media);
[[nodiscard]] QString PresentMediaNextAiring(const Media &media);
[[nodiscard]] QVariantList PresentMediaLinks(const Media &media);

#endif // HAIKENANIME_MEDIADETAILSPRESENTATION_H
