#ifndef HAIKENANIME_MEDIADETAILSPRESENTATION_H
#define HAIKENANIME_MEDIADETAILSPRESENTATION_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "../../domain/media/Media.h"

[[nodiscard]] QString PresentMediaSeason(const Media &media);
[[nodiscard]] QString PresentMediaNextAiring(const Media &media);
[[nodiscard]] QVariantList PresentMediaLinks(const Media &media);
/// The selected* fields MediaDetailsPanel reads, for screens that show a media without selecting it on the Home.
[[nodiscard]] QVariantMap PresentMediaDetails(const Media &media, const QString &title, const QString &coverSource);

#endif // HAIKENANIME_MEDIADETAILSPRESENTATION_H
