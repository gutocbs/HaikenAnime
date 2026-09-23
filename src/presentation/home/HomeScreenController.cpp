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
    : HomeScreenController(&repository, {}, parent) {
}

HomeScreenController::HomeScreenController(IMediaRepository *repository, QString initializationError, QObject *parent)
    : QObject(parent), repository_(repository), model_(this), errorMessage_(std::move(initializationError)) {
    if (repository_ == nullptr && !errorMessage_.isEmpty()) {
        state_ = QStringLiteral("error");
    }
}

HomeMediaModel *HomeScreenController::mediaModel() {
    return &model_;
}

QString HomeScreenController::state() const {
    return state_;
}

QString HomeScreenController::statusMessage() const {
    return statusMessage_;
}

QString HomeScreenController::errorMessage() const {
    return errorMessage_;
}

int HomeScreenController::synchronizationProgress() const {
    return synchronizationProgress_;
}

bool HomeScreenController::synchronizationProgressKnown() const {
    return synchronizationProgressKnown_;
}

int HomeScreenController::mediaCount() const {
    return model_.rowCount();
}

void HomeScreenController::reload() {
    if (repository_ == nullptr) {
        if (errorMessage_.isEmpty()) {
            errorMessage_ = QStringLiteral("Media repository is unavailable.");
            emit errorMessageChanged();
        }
        model_.setMedia({});
        emit mediaCountChanged();
        setState(QStringLiteral("error"));
        return;
    }

    setState(QStringLiteral("loading"));
    setStatusMessage(QStringLiteral("Carregando dados locais."));

    QString error;
    auto media = repository_->ReadAll(error);
    if (!error.isEmpty()) {
        if (errorMessage_ != error) {
            errorMessage_ = std::move(error);
            emit errorMessageChanged();
        }
        model_.setMedia({});
        emit mediaCountChanged();
        setState(QStringLiteral("error"));
        setStatusMessage(QStringLiteral("Não foi possível carregar os dados locais."));
        return;
    }

    if (!errorMessage_.isEmpty()) {
        errorMessage_.clear();
        emit errorMessageChanged();
    }
    model_.setMedia(std::move(media));
    emit mediaCountChanged();
    setState(model_.rowCount() == 0 ? QStringLiteral("empty") : QStringLiteral("ready"));
    setStatusMessage(model_.rowCount() == 0
                         ? QStringLiteral("Nenhum dado disponível.")
                         : QStringLiteral("Dados locais carregados."));
}

void HomeScreenController::notifySynchronizationCompleted() {
    synchronizationProgress_ = 100;
    synchronizationProgressKnown_ = true;
    emit synchronizationProgressChanged();
    setStatusMessage(QStringLiteral("Sincronização concluída. Atualizando dados."));
    emit synchronizationCompleted();
    emit mediaUpdated();
}

void HomeScreenController::notifySynchronizationStarted() {
    synchronizationProgress_ = 0;
    synchronizationProgressKnown_ = false;
    emit synchronizationProgressChanged();
    setState(QStringLiteral("loading"));
    setStatusMessage(QStringLiteral("Sincronizando dados em segundo plano..."));
}

void HomeScreenController::notifySynchronizationProgress(const int processedItems, const int totalItems) {
    if (totalItems <= 0) {
        if (synchronizationProgressKnown_) {
            synchronizationProgressKnown_ = false;
            emit synchronizationProgressChanged();
        }
        return;
    }

    const auto progress = qBound(0, (processedItems * 100) / totalItems, 100);
    if (synchronizationProgress_ == progress && synchronizationProgressKnown_) {
        return;
    }
    synchronizationProgress_ = progress;
    synchronizationProgressKnown_ = true;
    emit synchronizationProgressChanged();
}

void HomeScreenController::notifySynchronizationFailed(const QString &error) {
    synchronizationProgressKnown_ = false;
    emit synchronizationProgressChanged();
    errorMessage_ = error;
    emit errorMessageChanged();
    setState(QStringLiteral("error"));
    setStatusMessage(QStringLiteral("A sincronização falhou."));
    emit synchronizationError();
}

void HomeScreenController::setStatusMessage(QString message) {
    if (statusMessage_ == message) {
        return;
    }
    statusMessage_ = std::move(message);
    emit statusMessageChanged();
}

void HomeScreenController::setState(QString state) {
    if (state_ == state) {
        return;
    }
    state_ = std::move(state);
    emit stateChanged();
}
