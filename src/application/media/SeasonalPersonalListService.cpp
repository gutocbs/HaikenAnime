#include "SeasonalPersonalListService.h"
#include "PersonalListChangeService.h"

#include <algorithm>

namespace {
bool isPersonalListStatus(const UserListStatus status) {
    return status == UserListStatus::Current || status == UserListStatus::Planning
        || status == UserListStatus::OnHold || status == UserListStatus::Dropped
        || status == UserListStatus::Completed;
}
}

SeasonalPersonalListService::SeasonalPersonalListService(IMediaReader *reader, IMediaWriter *writer,
                                                         IPersonalListMediaWriter *personalListWriter,
                                                         PersonalListChangeService *personalListChangeService)
    : reader_(reader), writer_(writer), personalListWriter_(personalListWriter),
      personalListChangeService_(personalListChangeService) {}

bool SeasonalPersonalListService::find(const int mediaId, Media &media, bool &found,
                                       QString &error) const {
    media = {};
    found = false;
    error.clear();
    if (mediaId <= 0 || !reader_) {
        error = QStringLiteral("Local media repository is unavailable.");
        return false;
    }

    QList<Media> localMedia;
    if (!reader_->readAll(localMedia, error)) return false;
    const auto match = std::find_if(localMedia.cbegin(), localMedia.cend(), [mediaId](const Media &item) {
        return item.Id == mediaId;
    });
    if (match == localMedia.cend()) return true;
    media = *match;
    found = true;
    return true;
}

bool SeasonalPersonalListService::save(const Media &catalogMedia, const PersonalListMediaEdit &edit,
                                      Media &saved, bool &created, QString &error) const {
    saved = {};
    created = false;
    error.clear();
    if (!isPersonalListStatus(edit.status)) {
        error = QStringLiteral("Select one personal list before saving.");
        return false;
    }

    Media existing;
    bool found = false;
    if (!find(catalogMedia.Id, existing, found, error)) return false;
    if (found) {
        if (!personalListWriter_) {
            error = QStringLiteral("Local media repository is unavailable.");
            return false;
        }
        saved = existing;
        saved.ConsumedChapters = edit.progress;
        saved.PersonalScore = edit.score;
        saved.ListStatus = edit.status;
        saved.LocalPath = edit.path;
        saved.AlternativeNames = edit.alternativeNames;
        const bool savedLocally = personalListChangeService_
            ? personalListChangeService_->save(existing, saved, error)
            : personalListWriter_->updatePersonalListMedia(saved, error);
        if (!savedLocally) {
            saved = {};
            return false;
        }
        return true;
    }
    if (!writer_) {
        error = QStringLiteral("Local media repository is unavailable.");
        return false;
    }

    saved = catalogMedia;
    saved.ConsumedChapters = edit.progress;
    saved.PersonalScore = edit.score;
    saved.ListStatus = edit.status;
    saved.LocalPath = edit.path;
    saved.AlternativeNames = edit.alternativeNames;
    Media previous;
    previous.Id = saved.Id;
    const bool savedLocally = personalListChangeService_
        ? personalListChangeService_->save(previous, saved, error)
        : writer_->upsert({saved}, error);
    if (!savedLocally) {
        saved = {};
        return false;
    }
    created = true;
    return true;
}
