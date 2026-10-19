#include "EpisodeRecognitionController.h"

#include <algorithm>

EpisodeRecognitionController::EpisodeRecognitionController(RecognitionHistoryService &history,
                                                           IMediaReader &media, QObject *parent)
    : QObject(parent), history_(history), media_(media), model_(this) {}

void EpisodeRecognitionController::Reload() {
    refreshMedia();
    QList<RecognitionRecord> records;
    if (readPage(std::nullopt, records)) {
        errorMessage_.clear();
        model_.SetRecords(records, mediaTitles_);
    }
    emit changed();
}

void EpisodeRecognitionController::LoadMore() {
    if (!hasMore_ || model_.records().isEmpty()) return;
    QList<RecognitionRecord> records;
    if (readPage(model_.records().last().id(), records)) model_.AppendRecords(records, mediaTitles_);
    emit changed();
}

bool EpisodeRecognitionController::Remove(const qint64 recordId) {
    QString error;
    if (!history_.remove(recordId, error)) {
        errorMessage_ = error;
        emit changed();
        return false;
    }
    Reload();
    return true;
}

bool EpisodeRecognitionController::readPage(const std::optional<qint64> beforeId, QList<RecognitionRecord> &records) {
    QString error;
    // One extra row tells whether another page exists without a count query.
    if (!history_.list({}, {.limit = PageSize + 1, .beforeId = beforeId}, records, error)) {
        errorMessage_ = error;
        return false;
    }
    hasMore_ = records.size() > PageSize;
    if (hasMore_) records.removeLast();
    return true;
}

void EpisodeRecognitionController::refreshMedia() {
    QList<Media> media;
    QString error;
    if (!media_.readAll(media, error)) return;
    std::sort(media.begin(), media.end(), [](const Media &left, const Media &right) {
        return QString::localeAwareCompare(left.Name, right.Name) < 0;
    });
    mediaTitles_.clear();
    for (const auto &item : media) {
        mediaTitles_.insert(item.Id, item.Name);
    }
}
