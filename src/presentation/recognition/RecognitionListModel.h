#ifndef HAIKENANIME_RECOGNITIONLISTMODEL_H
#define HAIKENANIME_RECOGNITIONLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QList>

#include "../../domain/recognition/RecognitionRecord.h"

/// Recognition history rows for QML. Titles of library media are resolved by the controller.
class RecognitionListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        RecordIdRole = Qt::UserRole + 1,
        TitleRole,
        MediaIdRole,
        SeasonRole,
        EpisodeRole,
        StatusRole,
        PluginRole,
        UserRole,
        DeviceRole,
        DiagnosticRole,
        SyncStateRole,
        SyncErrorRole,
        UpdatedAtRole,
        CorrectableRole,
        CorrectedRole
    };

    explicit RecognitionListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void setRecords(const QList<RecognitionRecord> &records, const QHash<int, QString> &mediaTitles);
    void appendRecords(const QList<RecognitionRecord> &records, const QHash<int, QString> &mediaTitles);
    [[nodiscard]] const QList<RecognitionRecord> &records() const { return records_; }

private:
    QList<RecognitionRecord> records_;
    QHash<int, QString> mediaTitles_;
};

#endif // HAIKENANIME_RECOGNITIONLISTMODEL_H
