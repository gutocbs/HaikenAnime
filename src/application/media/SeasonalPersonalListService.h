#ifndef HAIKENANIME_SEASONALPERSONALLISTSERVICE_H
#define HAIKENANIME_SEASONALPERSONALLISTSERVICE_H

#include "IMediaReader.h"
#include "IMediaWriter.h"
#include "IPersonalListMediaWriter.h"

class PersonalListChangeService;

struct PersonalListMediaEdit final {
    int progress = 0;
    int score = 0;
    QString path;
    QStringList alternativeNames;
    UserListStatus status = UserListStatus::Unknown;
};

/** Adds catalog media to the local personal list without invoking remote AniList mutations. */
class SeasonalPersonalListService final {
public:
    SeasonalPersonalListService(IMediaReader *reader, IMediaWriter *writer,
                                IPersonalListMediaWriter *personalListWriter,
                                PersonalListChangeService *personalListChangeService = nullptr);

    [[nodiscard]] bool find(int mediaId, Media &media, bool &found, QString &error) const;
    [[nodiscard]] bool save(const Media &catalogMedia, const PersonalListMediaEdit &edit, Media &saved,
                           bool &created, QString &error) const;

private:
    IMediaReader *reader_ = nullptr;
    IMediaWriter *writer_ = nullptr;
    IPersonalListMediaWriter *personalListWriter_ = nullptr;
    PersonalListChangeService *personalListChangeService_ = nullptr;
};

#endif // HAIKENANIME_SEASONALPERSONALLISTSERVICE_H
