#include "RecognitionListModel.h"

RecognitionListModel::RecognitionListModel(QObject *parent) : QAbstractListModel(parent) {}

int RecognitionListModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : static_cast<int>(records_.size());
}

QVariant RecognitionListModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= records_.size()) return {};
    const auto &record = records_.at(index.row());
    const auto &candidate = record.effectiveCandidate();
    switch (role) {
    case RecordIdRole: return record.id();
    case TitleRole: {
        const auto mediaTitle = candidate.mediaId() ? mediaTitles_.value(*candidate.mediaId()) : QString();
        return mediaTitle.isEmpty() ? candidate.title() : mediaTitle;
    }
    case MediaIdRole: return candidate.mediaId() ? QVariant(*candidate.mediaId()) : QVariant();
    case SeasonRole: return candidate.season() ? QVariant(*candidate.season()) : QVariant();
    case EpisodeRole: return candidate.episode();
    case StatusRole: return ToString(record.status());
    case PluginRole: return record.sessionId().pluginId();
    case UserRole: return record.userId();
    case DeviceRole: return record.deviceId();
    case DiagnosticRole: return record.diagnostic();
    case SyncStateRole: return ToString(record.syncState());
    case SyncErrorRole: return record.syncError();
    case UpdatedAtRole: return record.updatedAt().toLocalTime().toString(QStringLiteral("dd/MM/yyyy HH:mm"));
    case CorrectableRole: return record.status() != RecognitionStatus::Pending;
    case CorrectedRole: return record.correction().has_value();
    default: return {};
    }
}

QHash<int, QByteArray> RecognitionListModel::roleNames() const {
    return {{RecordIdRole, "recordId"},     {TitleRole, "title"},          {MediaIdRole, "mediaId"},
            {SeasonRole, "season"},         {EpisodeRole, "episode"},      {StatusRole, "statusKey"},
            {PluginRole, "pluginId"},       {UserRole, "userId"},          {DeviceRole, "deviceId"},
            {DiagnosticRole, "diagnostic"}, {SyncStateRole, "syncState"},  {SyncErrorRole, "syncError"},
            {UpdatedAtRole, "updatedAt"},   {CorrectableRole, "correctable"}, {CorrectedRole, "corrected"}};
}

void RecognitionListModel::SetRecords(const QList<RecognitionRecord> &records, const QHash<int, QString> &mediaTitles) {
    beginResetModel();
    records_ = records;
    mediaTitles_ = mediaTitles;
    endResetModel();
}

void RecognitionListModel::AppendRecords(const QList<RecognitionRecord> &records,
                                         const QHash<int, QString> &mediaTitles) {
    mediaTitles_ = mediaTitles;
    if (records.isEmpty()) return;
    beginInsertRows({}, static_cast<int>(records_.size()), static_cast<int>(records_.size() + records.size() - 1));
    records_.append(records);
    endInsertRows();
}
