#ifndef HAIKENANIME_MEDIAPRESENTATION_H
#define HAIKENANIME_MEDIAPRESENTATION_H

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "../../domain/media/Media.h"

// Formats "consumed/total"; an unknown total (<= 0) is shown as "?".
[[nodiscard]] QString FormatProgress(int consumed, int total);
/// Translated release status ("Em lançamento", …), shared by every screen that shows media.
[[nodiscard]] QString MediaStatusLabel(MediaStatus status);
/// "Anime", "Manga", "Novel", or the translated "Desconhecido".
[[nodiscard]] QString MediaTypeLabel(MediaType type);
[[nodiscard]] QString PresentMediaSeason(const Media &media);
[[nodiscard]] QString PresentMediaNextAiring(const Media &media);
[[nodiscard]] QVariantList PresentMediaLinks(const Media &media);
/// The selected* fields MediaDetailsPanel reads, for screens that show a media without selecting it on the Home.
[[nodiscard]] QVariantMap PresentMediaDetails(const Media &media, const QString &title, const QString &coverSource);

#endif // HAIKENANIME_MEDIAPRESENTATION_H
