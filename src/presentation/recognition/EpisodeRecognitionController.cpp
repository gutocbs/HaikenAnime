#include "EpisodeRecognitionController.h"

#include <algorithm>

EpisodeRecognitionController::EpisodeRecognitionController(RecognitionHistoryService &history,
                                                           ProgressDecisionService &decisions, IMediaReader &media,
                                                           QObject *parent)
    : QObject(parent), history_(history), decisions_(decisions), media_(media), model_(this) {}

void EpisodeRecognitionController::Reload() {
    refreshMedia();
    QList<RecognitionRecord> records;
    if (readPage(std::nullopt, records)) {
        errorMessage_.clear();
        model_.setRecords(records, mediaTitles_);
    }
    emit changed();
}

void EpisodeRecognitionController::LoadMore() {
    if (!hasMore_ || model_.records().isEmpty()) return;
    QList<RecognitionRecord> records;
    if (readPage(model_.records().last().id(), records)) model_.appendRecords(records, mediaTitles_);
    emit changed();
}

bool EpisodeRecognitionController::Correct(const qint64 recordId, const int mediaId, const int episode) {
    const auto candidate = mediaTitles_.contains(mediaId)
        ? EpisodeCandidate::create(mediaTitles_.value(mediaId), mediaId, std::nullopt, episode, 1.0)
        : std::nullopt;
    if (!candidate.has_value()) {
        errorMessage_ = tr("Choose an anime from the library and an episode number from 1.");
        emit changed();
        return false;
    }
    // The decision keeps the same rules as automatic recognition: never regress progress.
    QString error;
    if (!history_.correct(recordId, *candidate, decisions_.evaluate(*candidate), error)) {
        errorMessage_ = error;
        emit changed();
        return false;
    }
    Reload();
    return true;
}

bool EpisodeRecognitionController::Ignore(const qint64 recordId) {
    QString error;
    if (!history_.ignore(recordId, error)) {
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
    mediaChoices_.clear();
    for (const auto &item : media) {
        mediaTitles_.insert(item.Id, item.Name);
        mediaChoices_.append(QVariantMap{{QStringLiteral("id"), item.Id}, {QStringLiteral("title"), item.Name}});
    }
}
