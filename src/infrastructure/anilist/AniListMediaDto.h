#ifndef HAIKENANIME_ANILISTMEDIADTO_H
#define HAIKENANIME_ANILISTMEDIADTO_H

#include <QString>
#include <QStringList>
#include <optional>

struct AniListCoverImagesDto {
    QString medium;
    QString large;
    QString extraLarge;
};

/** Represents the external AniList payload before domain mapping. */
struct AniListMediaDto {
    int id = 0;
    QString mediaType;
    QString mediaFormat;
    QString status;
    QString listStatus;
    QString titleRomaji;
    QString titleEnglish;
    QString titleNative;
    QStringList titleSynonyms;
    std::optional<int> episodes;
    std::optional<int> chapters;
    std::optional<int> progress;
    std::optional<int> personalScore;
    std::optional<int> averageScore;
    AniListCoverImagesDto coverImages;
    QString coverImageUrl;
    QString description;
};

#endif // HAIKENANIME_ANILISTMEDIADTO_H
