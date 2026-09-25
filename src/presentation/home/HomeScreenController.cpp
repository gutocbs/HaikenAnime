#include "HomeScreenController.h"

#include <QCoreApplication>
#include <QVariant>
#include <QUrl>

#include <utility>

namespace {
constexpr int MaximumHomeLibraryItems = 9;

QString mediaStatusLabel(const MediaStatus status) {
    switch (status) {
    case MediaStatus::NotReleased:
        return QCoreApplication::translate("HomeMediaModel", "Ainda não lançado");
    case MediaStatus::Releasing:
        return QCoreApplication::translate("HomeMediaModel", "Em lançamento");
    case MediaStatus::Released:
        return QCoreApplication::translate("HomeMediaModel", "Concluído");
    case MediaStatus::Unknown:
    default:
        return QCoreApplication::translate("HomeMediaModel", "Desconhecido");
    }
}
}

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
    case StatusLabelRole:
        return mediaStatusLabel(media.Status);
    case RemoteCoverUrlRole:
        return media.CoverUrl;
    case CoverSourceRole:
        return coverSources_.value(media.Id, QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"));
    case CoverStateRole:
        return static_cast<int>(coverStates_.value(media.Id, CoverState::Missing));
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
        {StatusLabelRole, "statusLabel"},
        {RemoteCoverUrlRole, "remoteCoverUrl"},
        {CoverSourceRole, "coverSource"},
        {CoverStateRole, "coverState"}
    };
}

bool HomeMediaModel::UpdateCover(int mediaId, QString source, CoverState state) {
    for (int row = 0; row < media_.size(); ++row) {
        if (media_[row].Id != mediaId) continue;
        if (!source.isEmpty()) coverSources_[mediaId] = std::move(source);
        else if (state == CoverState::Missing) coverSources_.remove(mediaId);
        coverStates_[mediaId] = state;
        emit dataChanged(index(row), index(row), {CoverSourceRole, CoverStateRole});
        return true;
    }
    return false;
}

QList<Media> HomeMediaModel::media() const { return media_; }

void HomeMediaModel::ClearCovers() {
    coverSources_.clear(); coverStates_.clear();
    if (!media_.isEmpty()) emit dataChanged(index(0), index(media_.size()-1), {CoverSourceRole, CoverStateRole});
}

void HomeMediaModel::setMedia(QList<Media> media) {
    if (media.size() > MaximumHomeLibraryItems) {
        media = media.mid(0, MaximumHomeLibraryItems);
    }
    beginResetModel();
    media_ = std::move(media);
    endResetModel();
}

HomeScreenController::HomeScreenController(IMediaReader &reader, QObject *parent)
    : HomeScreenController(&reader, {}, parent) {
}

HomeScreenController::HomeScreenController(IMediaReader *reader, QString initializationError, QObject *parent)
    : HomeScreenController(reader, nullptr, CoverQuality::Medium, std::move(initializationError), parent) {
}

HomeScreenController::HomeScreenController(IMediaReader *reader, CoverDownloadCoordinator *covers, CoverQuality quality,
                                           QString initializationError, QObject *parent)
    : QObject(parent), reader_(reader), model_(this), covers_(covers), coverQuality_(quality),
      errorMessage_(std::move(initializationError)) {
    if (reader_ == nullptr && !errorMessage_.isEmpty()) {
        state_ = QStringLiteral("error");
    }
    if (covers_) {
        connect(covers_, &CoverDownloadCoordinator::CoverAvailable, this, [this](int id, const QString &path) {
            model_.UpdateCover(id, QUrl::fromLocalFile(path).toString(), CoverState::Available);
        });
        connect(covers_, &CoverDownloadCoordinator::CoverStateChanged, this, [this](int id, CoverState state) {
            model_.UpdateCover(id, {}, state);
        });
        connect(covers_, &CoverDownloadCoordinator::ClearCompleted, &model_, &HomeMediaModel::ClearCovers);
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
    if (reader_ == nullptr) {
        if (errorMessage_.isEmpty()) {
            errorMessage_ = QStringLiteral("Media repository is unavailable.");
            emit errorMessageChanged();
        }
        model_.setMedia({});
        emit mediaCountChanged();
        setState(QStringLiteral("error"));
        setStatusMessage(QStringLiteral("Não foi possível carregar os dados locais."));
        return;
    }

    setState(QStringLiteral("loading"));
    setStatusMessage(QStringLiteral("Carregando dados locais."));

    QString error;
    QList<Media> media;
    if (!reader_->readAll(media, error)) {
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
    reload();
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
}

void HomeScreenController::RequestCoverWindow(int firstVisibleIndex, int lastVisibleIndex, int prefetchCount) {
    if (!covers_) return;
    const auto rows = model_.media();
    if (rows.isEmpty()) return;
    const int first = qBound(0, firstVisibleIndex, rows.size()-1);
    const int last = qBound(first, lastVisibleIndex, rows.size()-1);
    QList<CoverRequest> visible, prefetch;
    auto request = [&](int index, CoverPriority priority) {
        const auto &media = rows[index];
        if (!media.CoverUrl.isEmpty()) (priority == CoverPriority::Visible ? visible : prefetch)
            .append({media.Id, QUrl(media.CoverUrl), coverQuality_, priority, 0});
    };
    for (int i=first;i<=last;++i) request(i,CoverPriority::Visible);
    for (int i=last+1;i<qMin(rows.size(),last+1+qMax(0,prefetchCount));++i) request(i,CoverPriority::Prefetch);
    covers_->RequestWindow(std::move(visible), std::move(prefetch));
}

void HomeScreenController::ReportCoverLoadFailure(int mediaId) { if (covers_) covers_->ReportMissingFile(mediaId); }
void HomeScreenController::ClearCoverCache() { if (covers_) covers_->Clear(); }

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
