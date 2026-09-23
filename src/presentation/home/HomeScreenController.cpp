#include "HomeScreenController.h"

#include <QVariant>

#include <utility>

HomeMediaModel::HomeMediaModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int HomeMediaModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return media_.size();
}

QVariant HomeMediaModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= media_.size()) {
        return {};
    }

    const auto &media = media_.at(index.row());
    switch (role) {
    case IdRole:
        return media.Id;
    case TitleRole:
        return media.Name;
    case ProgressRole:
        return QStringLiteral("%1/%2").arg(media.ConsumedChapters).arg(media.TotalChapters);
    case ScoreRole:
        return media.PersonalScore > 0 ? QString::number(media.PersonalScore) : QStringLiteral("—");
    case StatusRole:
        return static_cast<int>(media.Status);
    case CoverUrlRole:
        return media.CoverUrl;
    default:
        return {};
    }
}

QHash<int, QByteArray> HomeMediaModel::roleNames() const {
    return {
        {IdRole, "mediaId"},
        {TitleRole, "title"},
        {ProgressRole, "progress"},
        {ScoreRole, "score"},
        {StatusRole, "status"},
        {CoverUrlRole, "coverUrl"}
    };
}

void HomeMediaModel::setMedia(QList<Media> media) {
    beginResetModel();
    media_ = std::move(media);
    endResetModel();
}

HomeScreenController::HomeScreenController(IMediaRepository &repository, QObject *parent)
    : QObject(parent), repository_(repository), model_(this) {
}

HomeMediaModel *HomeScreenController::mediaModel() {
    return &model_;
}

QString HomeScreenController::state() const {
    return state_;
}

QString HomeScreenController::errorMessage() const {
    return errorMessage_;
}

int HomeScreenController::mediaCount() const {
    return model_.rowCount();
}

void HomeScreenController::reload() {
    setState(QStringLiteral("loading"));

    QString error;
    auto media = repository_.ReadAll(error);
    if (!error.isEmpty()) {
        if (errorMessage_ != error) {
            errorMessage_ = std::move(error);
            emit errorMessageChanged();
        }
        model_.setMedia({});
        emit mediaCountChanged();
        setState(QStringLiteral("error"));
        return;
    }

    if (!errorMessage_.isEmpty()) {
        errorMessage_.clear();
        emit errorMessageChanged();
    }
    model_.setMedia(std::move(media));
    emit mediaCountChanged();
    setState(model_.rowCount() == 0 ? QStringLiteral("empty") : QStringLiteral("ready"));
}

void HomeScreenController::setState(QString state) {
    if (state_ == state) {
        return;
    }
    state_ = std::move(state);
    emit stateChanged();
}
