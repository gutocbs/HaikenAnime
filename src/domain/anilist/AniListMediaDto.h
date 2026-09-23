#ifndef HAIKENANIME_ANILISTMEDIADTO_H
#define HAIKENANIME_ANILISTMEDIADTO_H

#include <QString>
#include <QStringList>

/** Represents the external AniList shape before domain mapping. */
struct AniListMediaDto {
    int id = 0;
    QString type;
    QString status;
    QString titleRomaji;
    QString titleEnglish;
    QString titleNative;
    QStringList titleSynonyms;
    int episodes = 0;
    int chapters = 0;
    int averageScore = 0;
    QString coverImageUrl;
    QString description;
};

#endif // HAIKENANIME_ANILISTMEDIADTO_H
