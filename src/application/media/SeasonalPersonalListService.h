#ifndef HAIKENANIME_SEASONALPERSONALLISTSERVICE_H
#define HAIKENANIME_SEASONALPERSONALLISTSERVICE_H

#include "IMediaReader.h"
#include "IMediaWriter.h"

/** Adds catalog media to the local personal list without invoking remote AniList mutations. */
class SeasonalPersonalListService final {
public:
    SeasonalPersonalListService(IMediaReader *reader, IMediaWriter *writer);

    [[nodiscard]] bool find(int mediaId, Media &media, bool &found, QString &error) const;
    [[nodiscard]] bool add(const Media &catalogMedia, UserListStatus status, Media &saved,
                           bool &created, QString &error) const;

private:
    IMediaReader *reader_ = nullptr;
    IMediaWriter *writer_ = nullptr;
};

#endif // HAIKENANIME_SEASONALPERSONALLISTSERVICE_H
