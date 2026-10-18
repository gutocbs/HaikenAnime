#ifndef HAIKENANIME_EPISODERECOGNITIONCONTROLLER_H
#define HAIKENANIME_EPISODERECOGNITIONCONTROLLER_H

#include <QObject>
#include <QVariantList>

#include "../../application/media/IMediaReader.h"
#include "../../application/recognition/RecognitionHistoryService.h"
#include "RecognitionListModel.h"

/// Recognition history. A status panel, not a confirmation queue: progress has already been applied
/// automatically; the user can only delete entries from the local history.
class EpisodeRecognitionController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject *records READ records CONSTANT)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY changed)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY changed)

public:
    static constexpr int PageSize = 50;

    EpisodeRecognitionController(RecognitionHistoryService &history, IMediaReader &media, QObject *parent = nullptr);

    [[nodiscard]] QObject *records() { return &model_; }
    [[nodiscard]] RecognitionListModel &model() { return model_; }
    [[nodiscard]] bool hasMore() const { return hasMore_; }
    [[nodiscard]] QString errorMessage() const { return errorMessage_; }

    Q_INVOKABLE void Reload();
    Q_INVOKABLE void LoadMore();
    /// Deletes the record from the local history (and its unsent AniList progress change).
    Q_INVOKABLE bool Remove(qint64 recordId);

signals:
    void changed();

private:
    bool readPage(std::optional<qint64> beforeId, QList<RecognitionRecord> &records);
    void refreshMedia();

    RecognitionHistoryService &history_;
    IMediaReader &media_;
    RecognitionListModel model_;
    QHash<int, QString> mediaTitles_;
    QString errorMessage_;
    bool hasMore_ = false;
};

#endif // HAIKENANIME_EPISODERECOGNITIONCONTROLLER_H
