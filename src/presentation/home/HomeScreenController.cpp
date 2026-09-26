#include "HomeScreenController.h"

#include <QCoreApplication>
#include <QVariant>
#include <QUrl>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace {
constexpr int MaximumHomeLibraryItems = 9;
const QString CoverPlaceholder = QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg");

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

QString mediaTypeKey(const MediaType type) {
    switch (type) {
    case MediaType::Anime: return QStringLiteral("anime");
    case MediaType::Manga: return QStringLiteral("manga");
    case MediaType::Novel: return QStringLiteral("novel");
    case MediaType::Unknown:
    default: return QStringLiteral("unknown");
    }
}

QString mediaTypeLabel(const MediaType type) {
    switch (type) {
    case MediaType::Anime: return QStringLiteral("Anime");
    case MediaType::Manga: return QStringLiteral("Manga");
    case MediaType::Novel: return QStringLiteral("Novel");
    case MediaType::Unknown:
    default: return QCoreApplication::translate("HomeMediaModel", "Desconhecido");
    }
}

QString mediaProgress(const Media &media) {
    return QStringLiteral("%1/%2").arg(media.ConsumedChapters).arg(media.TotalChapters);
}

QString personalScore(const Media &media) {
    return media.PersonalScore > 0 ? QString::number(media.PersonalScore) : QStringLiteral("—");
}

QString userListStatusKey(const UserListStatus status) {
    switch (status) {
    case UserListStatus::Current: return QStringLiteral("current");
    case UserListStatus::Planning: return QStringLiteral("planning");
    case UserListStatus::OnHold: return QStringLiteral("on_hold");
    case UserListStatus::Dropped: return QStringLiteral("dropped");
    case UserListStatus::Completed: return QStringLiteral("completed");
    case UserListStatus::Unknown:
    default: return QStringLiteral("unknown");
    }
}

QVariantMap option(const QString &key, const QString &label) {
    return {{QStringLiteral("key"), key}, {QStringLiteral("label"), label}};
}

QVariantList defaultListOptions(const QString &mediaType) {
    const auto currentLabel = mediaType == QStringLiteral("anime")
        ? QCoreApplication::translate("HomeScreenController", "Assistindo")
        : QCoreApplication::translate("HomeScreenController", "Lendo");
    return {
        option(QStringLiteral("all"), QCoreApplication::translate("HomeScreenController", "Todas")),
        option(QStringLiteral("current"), currentLabel),
        option(QStringLiteral("planning"), QCoreApplication::translate("HomeScreenController", "Planejando")),
        option(QStringLiteral("on_hold"), QCoreApplication::translate("HomeScreenController", "Em pausa")),
        option(QStringLiteral("dropped"), QCoreApplication::translate("HomeScreenController", "Abandonadas")),
        option(QStringLiteral("completed"), QCoreApplication::translate("HomeScreenController", "Concluídas"))
    };
}

QVariantList defaultMediaTypeOptions() {
    return {
        option(QStringLiteral("anime"), QStringLiteral("Anime")),
        option(QStringLiteral("manga"), QStringLiteral("Manga")),
        option(QStringLiteral("novel"), QStringLiteral("Novel"))
    };
}

QVariantList defaultSortOptions() {
    return {
        option(QStringLiteral("title_asc"), QCoreApplication::translate("HomeScreenController", "Título A–Z")),
        option(QStringLiteral("title_desc"), QCoreApplication::translate("HomeScreenController", "Título Z–A")),
        option(QStringLiteral("personal_score"), QCoreApplication::translate("HomeScreenController", "Maior nota")),
        option(QStringLiteral("progress"), QCoreApplication::translate("HomeScreenController", "Maior progresso"))
    };
}

bool matchesSearch(const Media &media, const QString &query) {
    if (query.isEmpty()) return true;
    if (media.Name.contains(query, Qt::CaseInsensitive)
        || media.EnglishName.contains(query, Qt::CaseInsensitive)
        || media.OriginalName.contains(query, Qt::CaseInsensitive)) {
        return true;
    }
    return std::any_of(media.AlternativeNames.cbegin(), media.AlternativeNames.cend(),
                       [&query](const QString &title) {
                           return title.contains(query, Qt::CaseInsensitive);
                       });
}

bool containsOptionKey(const QVariantList &options, const QString &key) {
    return std::any_of(options.cbegin(), options.cend(), [&key](const QVariant &entry) {
        return entry.toMap().value(QStringLiteral("key")).toString() == key;
    });
}

QString firstOptionKey(const QVariantList &options) {
    return options.first().toMap().value(QStringLiteral("key")).toString();
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
        return mediaProgress(media);
    case ScoreRole:
        return personalScore(media);
    case StatusLabelRole:
        return mediaStatusLabel(media.Status);
    case RemoteCoverUrlRole:
        return media.CoverUrl;
    case CoverSourceRole:
        return coverSources_.value(media.Id, CoverPlaceholder);
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
    : QObject(parent), reader_(reader), model_(this), fullModel_(this), covers_(covers), coverQuality_(quality),
      errorMessage_(std::move(initializationError)) {
    mediaTypeOptions_ = defaultMediaTypeOptions();
    listOptions_ = defaultListOptions(activeMediaType_);
    sortOptions_ = defaultSortOptions();
    if (reader_ == nullptr && !errorMessage_.isEmpty()) {
        state_ = QStringLiteral("error");
    }
    if (covers_) {
        connect(covers_, &CoverDownloadCoordinator::CoverAvailable, this, [this](int id, const QString &path) {
            model_.UpdateCover(id, QUrl::fromLocalFile(path).toString(), CoverState::Available);
            fullModel_.UpdateCover(id, QUrl::fromLocalFile(path).toString(), CoverState::Available);
            if (hasSelection_ && selectedMedia_.Id == id) {
                selectedCoverSource_ = QUrl::fromLocalFile(path).toString();
                emit selectionChanged();
            }
        });
        connect(covers_, &CoverDownloadCoordinator::CoverStateChanged, this, [this](int id, CoverState state) {
            model_.UpdateCover(id, {}, state);
            fullModel_.UpdateCover(id, {}, state);
            if (state == CoverState::Missing && hasSelection_ && selectedMedia_.Id == id) {
                selectedCoverSource_ = CoverPlaceholder;
                emit selectionChanged();
            }
        });
        connect(covers_, &CoverDownloadCoordinator::ClearCompleted, this, [this] {
            model_.ClearCovers();
            fullModel_.ClearCovers();
            if (hasSelection_) {
                selectedCoverSource_ = CoverPlaceholder;
                emit selectionChanged();
            }
        });
    }
}

HomeMediaModel *HomeScreenController::mediaModel() {
    return &model_;
}

HomeMediaModel *HomeScreenController::fullMediaModel() {
    return &fullModel_;
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
    return allMedia_.size();
}

int HomeScreenController::filteredMediaCount() const {
    return fullModel_.rowCount();
}

QString HomeScreenController::activeMediaType() const { return activeMediaType_; }
QVariantList HomeScreenController::availableMediaTypeOptions() const { return mediaTypeOptions_; }
QVariantList HomeScreenController::availableListOptions() const {
    return listOptions_;
}
QVariantList HomeScreenController::availableSortOptions() const {
    return sortOptions_;
}
QString HomeScreenController::activeListFilter() const { return activeListFilter_; }
QString HomeScreenController::activeSort() const { return activeSort_; }
QString HomeScreenController::searchQuery() const { return searchQuery_; }
bool HomeScreenController::browseCriteriaActive() const {
    return activeListFilter_ != firstOptionKey(listOptions_)
        || activeSort_ != firstOptionKey(sortOptions_) || !searchQuery_.isEmpty();
}
bool HomeScreenController::hasSelection() const { return hasSelection_; }
int HomeScreenController::selectedMediaId() const { return hasSelection_ ? selectedMedia_.Id : 0; }
QString HomeScreenController::selectedTitle() const { return hasSelection_ ? selectedMedia_.Name : QString(); }
QString HomeScreenController::selectedSynopsis() const { return hasSelection_ ? selectedMedia_.Synopsis : QString(); }
QString HomeScreenController::selectedTypeLabel() const { return hasSelection_ ? mediaTypeLabel(selectedMedia_.Type) : QString(); }
QString HomeScreenController::selectedStatusLabel() const { return hasSelection_ ? mediaStatusLabel(selectedMedia_.Status) : QString(); }
QString HomeScreenController::selectedProgress() const { return hasSelection_ ? mediaProgress(selectedMedia_) : QString(); }
QString HomeScreenController::selectedScore() const { return hasSelection_ ? personalScore(selectedMedia_) : QString(); }
QString HomeScreenController::selectedAverageScore() const {
    return hasSelection_ && selectedMedia_.AverageScore > 0 ? QString::number(selectedMedia_.AverageScore) : QStringLiteral("—");
}
QString HomeScreenController::selectedCoverSource() const { return selectedCoverSource_; }

void HomeScreenController::ConfigureBrowseOptions(QVariantList mediaTypeOptions,
                                                  QVariantList listOptions,
                                                  QVariantList sortOptions) {
    if (mediaTypeOptions.isEmpty() || listOptions.isEmpty() || sortOptions.isEmpty()) return;
    const auto previousMediaType = activeMediaType_;
    const auto previousListFilter = activeListFilter_;
    const auto previousSort = activeSort_;
    mediaTypeOptions_ = std::move(mediaTypeOptions);
    listOptions_ = std::move(listOptions);
    sortOptions_ = std::move(sortOptions);
    if (!containsOptionKey(mediaTypeOptions_, activeMediaType_))
        activeMediaType_ = firstOptionKey(mediaTypeOptions_);
    if (!containsOptionKey(listOptions_, activeListFilter_))
        activeListFilter_ = firstOptionKey(listOptions_);
    if (!containsOptionKey(sortOptions_, activeSort_))
        activeSort_ = firstOptionKey(sortOptions_);
    usesDefaultBrowseOptions_ = false;
    if (activeMediaType_ != previousMediaType) {
        clearSelection();
        emit activeMediaTypeChanged();
    }
    if (activeMediaType_ != previousMediaType || activeListFilter_ != previousListFilter
        || activeSort_ != previousSort) {
        rebuildMediaModels();
        emit browseCriteriaChanged();
        emit mediaCountChanged();
    }
    emit browseOptionsChanged();
}

void HomeScreenController::SetMediaType(const QString &typeKey) {
    if (!containsOptionKey(mediaTypeOptions_, typeKey)) return;
    if (activeMediaType_ == typeKey) return;
    activeMediaType_ = typeKey;
    if (usesDefaultBrowseOptions_) listOptions_ = defaultListOptions(activeMediaType_);
    clearSelection();
    rebuildMediaModels();
    emit activeMediaTypeChanged();
    emit browseOptionsChanged();
    emit mediaCountChanged();
}

void HomeScreenController::SetListFilter(const QString &filterKey) {
    if (!containsOptionKey(listOptions_, filterKey) || activeListFilter_ == filterKey) return;
    activeListFilter_ = filterKey;
    rebuildMediaModels();
    emit browseCriteriaChanged();
    emit mediaCountChanged();
}

void HomeScreenController::SetSort(const QString &sortKey) {
    if (!containsOptionKey(sortOptions_, sortKey) || activeSort_ == sortKey) return;
    activeSort_ = sortKey;
    rebuildMediaModels();
    emit browseCriteriaChanged();
    emit mediaCountChanged();
}

void HomeScreenController::SetSearchQuery(const QString &query) {
    const auto normalized = query.trimmed();
    if (searchQuery_ == normalized) return;
    searchQuery_ = normalized;
    rebuildMediaModels();
    emit browseCriteriaChanged();
    emit mediaCountChanged();
}

void HomeScreenController::ClearBrowseCriteria() {
    const auto defaultListFilter = firstOptionKey(listOptions_);
    const auto defaultSort = firstOptionKey(sortOptions_);
    if (activeListFilter_ == defaultListFilter
        && activeSort_ == defaultSort && searchQuery_.isEmpty()) return;
    activeListFilter_ = defaultListFilter;
    activeSort_ = defaultSort;
    searchQuery_.clear();
    rebuildMediaModels();
    emit browseCriteriaChanged();
    emit mediaCountChanged();
}

void HomeScreenController::SelectMedia(const int mediaId) {
    const auto match = std::find_if(allMedia_.cbegin(), allMedia_.cend(), [mediaId](const Media &media) {
        return media.Id == mediaId;
    });
    if (match == allMedia_.cend() || mediaTypeKey(match->Type) != activeMediaType_) return;
    selectedMedia_ = *match;
    hasSelection_ = true;
    selectedCoverSource_ = CoverPlaceholder;
    const auto rows = fullModel_.media();
    for (int row = 0; row < rows.size(); ++row) {
        if (rows[row].Id == mediaId) {
            selectedCoverSource_ = fullModel_.data(fullModel_.index(row), HomeMediaModel::CoverSourceRole).toString();
            break;
        }
    }
    emit selectionChanged();
}

void HomeScreenController::reload() {
    if (reader_ == nullptr) {
        if (errorMessage_.isEmpty()) {
            errorMessage_ = QStringLiteral("Media repository is unavailable.");
            emit errorMessageChanged();
        }
        allMedia_.clear();
        rebuildMediaModels();
        clearSelection();
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
        allMedia_.clear();
        rebuildMediaModels();
        clearSelection();
        emit mediaCountChanged();
        setState(QStringLiteral("error"));
        setStatusMessage(QStringLiteral("Não foi possível carregar os dados locais."));
        return;
    }

    if (!errorMessage_.isEmpty()) {
        errorMessage_.clear();
        emit errorMessageChanged();
    }
    allMedia_ = std::move(media);
    rebuildMediaModels();
    clearSelection();
    emit mediaCountChanged();
    setState(allMedia_.isEmpty() ? QStringLiteral("empty") : QStringLiteral("ready"));
    setStatusMessage(allMedia_.isEmpty()
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

void HomeScreenController::RequestCoverWindow(const QString &scope, int firstVisibleIndex,
                                              int lastVisibleIndex, int prefetchCount) {
    if (!covers_) return;
    const auto rows = scope == QStringLiteral("full") ? fullModel_.media() : model_.media();
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

void HomeScreenController::rebuildMediaModels() {
    QList<Media> filtered;
    for (const auto &media : std::as_const(allMedia_)) {
        if (mediaTypeKey(media.Type) != activeMediaType_) continue;
        if (activeListFilter_ != QStringLiteral("all")
            && userListStatusKey(media.ListStatus) != activeListFilter_) continue;
        if (!matchesSearch(media, searchQuery_)) continue;
        filtered.append(media);
    }
    const auto titleLess = [](const Media &left, const Media &right) {
        return QString::compare(left.Name, right.Name, Qt::CaseInsensitive) < 0;
    };
    std::stable_sort(filtered.begin(), filtered.end(), [this, &titleLess](const Media &left, const Media &right) {
        if (activeSort_ == QStringLiteral("title_desc")) return titleLess(right, left);
        if (activeSort_ == QStringLiteral("personal_score") && left.PersonalScore != right.PersonalScore)
            return left.PersonalScore > right.PersonalScore;
        if (activeSort_ == QStringLiteral("progress") && left.ConsumedChapters != right.ConsumedChapters)
            return left.ConsumedChapters > right.ConsumedChapters;
        return titleLess(left, right);
    });
    fullModel_.setMedia(filtered);
    model_.setMedia(filtered.mid(0, MaximumHomeLibraryItems));
    if (hasSelection_) {
        const bool selectionVisible = std::any_of(filtered.cbegin(), filtered.cend(), [this](const Media &media) {
            return media.Id == selectedMedia_.Id;
        });
        if (!selectionVisible) clearSelection();
    }
}

void HomeScreenController::clearSelection() {
    if (!hasSelection_) return;
    hasSelection_ = false;
    selectedMedia_ = {};
    selectedCoverSource_ = CoverPlaceholder;
    emit selectionChanged();
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
