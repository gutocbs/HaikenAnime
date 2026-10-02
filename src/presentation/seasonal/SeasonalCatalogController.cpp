#include "SeasonalCatalogController.h"

#include "../../app/SeasonalCatalogCoordinator.h"
#include "../../application/covers/CoverSourceResolver.h"
#include "../../application/media/MediaDetailsPresentation.h"
#include "../../application/media/SeasonalPersonalListService.h"

#include <QCoreApplication>

#include <algorithm>
#include <utility>

namespace {
QString stateName(const SeasonalCatalogState state) {
    switch (state) {
    case SeasonalCatalogState::Loading: return QStringLiteral("loading");
    case SeasonalCatalogState::Populated: return QStringLiteral("populated");
    case SeasonalCatalogState::Empty: return QStringLiteral("empty");
    case SeasonalCatalogState::Error: return QStringLiteral("error");
    case SeasonalCatalogState::Idle:
    default: return QStringLiteral("idle");
    }
}

QString mediaTypeLabel(const MediaType type) {
    switch (type) {
    case MediaType::Anime: return QStringLiteral("Anime");
    case MediaType::Manga: return QStringLiteral("Manga");
    case MediaType::Novel: return QStringLiteral("Novel");
    case MediaType::Unknown:
    default: return QCoreApplication::translate("SeasonalCatalogController", "Desconhecido");
    }
}

QString mediaStatusLabel(const MediaStatus status) {
    switch (status) {
    case MediaStatus::NotReleased:
        return QCoreApplication::translate("SeasonalCatalogController", "Ainda não lançado");
    case MediaStatus::Releasing:
        return QCoreApplication::translate("SeasonalCatalogController", "Em lançamento");
    case MediaStatus::Released:
        return QCoreApplication::translate("SeasonalCatalogController", "Concluído");
    case MediaStatus::Unknown:
    default: return QCoreApplication::translate("SeasonalCatalogController", "Desconhecido");
    }
}

QString personalListStatusKey(const UserListStatus status) {
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

UserListStatus personalListStatusFromKey(const QString &key) {
    if (key == QStringLiteral("current")) return UserListStatus::Current;
    if (key == QStringLiteral("planning")) return UserListStatus::Planning;
    if (key == QStringLiteral("on_hold")) return UserListStatus::OnHold;
    if (key == QStringLiteral("dropped")) return UserListStatus::Dropped;
    if (key == QStringLiteral("completed")) return UserListStatus::Completed;
    return UserListStatus::Unknown;
}

QVariantMap personalListOption(const QString &key, const QString &label) {
    return {{QStringLiteral("key"), key}, {QStringLiteral("label"), label}};
}

QVariantList personalListOptions() {
    return {
        personalListOption(QStringLiteral("current"),
                           QCoreApplication::translate("SeasonalCatalogController", "Em andamento")),
        personalListOption(QStringLiteral("planning"),
                           QCoreApplication::translate("SeasonalCatalogController", "Planejando")),
        personalListOption(QStringLiteral("on_hold"),
                           QCoreApplication::translate("SeasonalCatalogController", "Em pausa")),
        personalListOption(QStringLiteral("dropped"),
                           QCoreApplication::translate("SeasonalCatalogController", "Abandonado")),
        personalListOption(QStringLiteral("completed"),
                           QCoreApplication::translate("SeasonalCatalogController", "Concluído"))
    };
}

}

SeasonalCatalogMediaModel::SeasonalCatalogMediaModel(QObject *parent) : QAbstractListModel(parent) {}

int SeasonalCatalogMediaModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : media_.size();
}

QVariant SeasonalCatalogMediaModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= media_.size()) return {};
    const auto &media = media_.at(index.row());
    switch (role) {
    case MediaIdRole: return media.Id;
    case TitleRole: return ResolveMediaTitle(media, preferredTitleKey_);
    case StatusLabelRole: return mediaStatusLabel(media.Status);
    case ProgressRole: return PresentMediaSeason(media);
    case ScoreRole:
        return QCoreApplication::translate("SeasonalCatalogController", "Nota AniList: %1")
            .arg(media.AverageScore > 0 ? QString::number(media.AverageScore) : QStringLiteral("—"));
    case CoverSourceRole: return ResolveCoverSource(media, coverQuality_);
    default: return {};
    }
}

QHash<int, QByteArray> SeasonalCatalogMediaModel::roleNames() const {
    return {{MediaIdRole, "mediaId"}, {TitleRole, "title"}, {StatusLabelRole, "statusLabel"},
            {ProgressRole, "progress"}, {ScoreRole, "score"}, {CoverSourceRole, "coverSource"}};
}

void SeasonalCatalogMediaModel::setMedia(QList<Media> media) {
    beginResetModel();
    media_ = std::move(media);
    endResetModel();
}

void SeasonalCatalogMediaModel::configurePresentation(const CoverQuality quality, QString preferredTitleKey) {
    preferredTitleKey = NormalizePreferredTitleKey(std::move(preferredTitleKey));
    if (coverQuality_ == quality && preferredTitleKey_ == preferredTitleKey) return;
    coverQuality_ = quality;
    preferredTitleKey_ = std::move(preferredTitleKey);
    if (!media_.isEmpty()) emit dataChanged(index(0), index(media_.size() - 1));
}

SeasonalCatalogController::SeasonalCatalogController(SeasonalCatalogCoordinator *coordinator,
                                                     const CoverQuality quality, QString preferredTitleKey,
                                                     QObject *parent,
                                                     SeasonalPersonalListService *personalLists)
    : QObject(parent), coordinator_(coordinator), mediaModel_(this), coverQuality_(quality),
      preferredTitleKey_(NormalizePreferredTitleKey(std::move(preferredTitleKey))),
      personalLists_(personalLists) {
    mediaModel_.configurePresentation(coverQuality_, preferredTitleKey_);
    if (coordinator_) {
        connect(coordinator_, &SeasonalCatalogCoordinator::changed, this,
                &SeasonalCatalogController::synchronizeFromCoordinator);
        synchronizeFromCoordinator();
    }
}

SeasonalCatalogMediaModel *SeasonalCatalogController::mediaModel() { return &mediaModel_; }

QVariantList SeasonalCatalogController::availableYearOptions() const {
    return coordinator_ ? coordinator_->availableYearOptions() : QVariantList{};
}

QVariantList SeasonalCatalogController::availableSeasonOptions() const {
    return coordinator_ ? coordinator_->availableSeasonOptions() : QVariantList{};
}

int SeasonalCatalogController::selectedYear() const { return coordinator_ ? coordinator_->year() : 0; }
QString SeasonalCatalogController::selectedSeasonKey() const { return coordinator_ ? coordinator_->seasonKey() : QString(); }
QString SeasonalCatalogController::state() const { return coordinator_ ? stateName(coordinator_->state()) : QStringLiteral("error"); }
QString SeasonalCatalogController::errorMessage() const { return coordinator_ ? coordinator_->error() : QStringLiteral("Seasonal catalog is unavailable."); }
bool SeasonalCatalogController::canLoadNextPage() const { return coordinator_ && coordinator_->canLoadNextPage(); }
bool SeasonalCatalogController::hasResults() const { return !media_.isEmpty(); }
bool SeasonalCatalogController::includeAdultContent() const { return coordinator_ && coordinator_->includeAdultContent(); }
bool SeasonalCatalogController::hasSelection() const { return selectedMedia() != nullptr; }
int SeasonalCatalogController::selectedMediaId() const { return hasSelection() ? selectedMediaId_ : 0; }
QString SeasonalCatalogController::selectedTitle() const { const auto *media = selectedMedia(); return media ? ResolveMediaTitle(*media, preferredTitleKey_) : QString(); }
QString SeasonalCatalogController::selectedSynopsis() const { const auto *media = selectedMedia(); return media ? media->Synopsis : QString(); }
QString SeasonalCatalogController::selectedTypeLabel() const { const auto *media = selectedMedia(); return media ? mediaTypeLabel(media->Type) : QString(); }
QString SeasonalCatalogController::selectedStatusLabel() const { const auto *media = selectedMedia(); return media ? mediaStatusLabel(media->Status) : QString(); }
QString SeasonalCatalogController::selectedProgress() const {
    if (!hasSelection()) return {};
    if (!selectedLocalMedia_) return QStringLiteral("—");
    return QStringLiteral("%1/%2").arg(selectedLocalMedia_->ConsumedChapters)
        .arg(selectedMedia()->TotalChapters);
}
QString SeasonalCatalogController::selectedScore() const {
    if (!hasSelection()) return {};
    return selectedLocalMedia_ && selectedLocalMedia_->PersonalScore > 0
        ? QString::number(selectedLocalMedia_->PersonalScore) : QStringLiteral("—");
}
QString SeasonalCatalogController::selectedAverageScore() const { const auto *media = selectedMedia(); return media && media->AverageScore > 0 ? QString::number(media->AverageScore) : hasSelection() ? QStringLiteral("—") : QString(); }
QString SeasonalCatalogController::selectedSeasonLabel() const { const auto *media = selectedMedia(); return media ? PresentMediaSeason(*media) : QString(); }
QString SeasonalCatalogController::selectedNextAiringLabel() const { const auto *media = selectedMedia(); return media ? PresentMediaNextAiring(*media) : QString(); }
QVariantList SeasonalCatalogController::selectedMediaLinks() const {
    const auto *media = selectedMedia();
    return media ? PresentMediaLinks(*media) : QVariantList{};
}
QString SeasonalCatalogController::selectedCoverSource() const { const auto *media = selectedMedia(); return media ? ResolveCoverSource(*media, coverQuality_) : QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"); }
QStringList SeasonalCatalogController::selectedAlternativeNames() const {
    const auto *media = selectedLocalMedia_ ? &*selectedLocalMedia_ : selectedMedia();
    return media ? media->AlternativeNames : QStringList{};
}
QVariantList SeasonalCatalogController::availablePersonalListOptions() const { return personalListOptions(); }
bool SeasonalCatalogController::selectedMediaInPersonalList() const { return selectedLocalMedia_.has_value(); }
QString SeasonalCatalogController::selectedListStatusKey() const {
    return selectedLocalMedia_ ? personalListStatusKey(selectedLocalMedia_->ListStatus) : QString();
}
int SeasonalCatalogController::selectedProgressValue() const {
    return selectedLocalMedia_ ? selectedLocalMedia_->ConsumedChapters : 0;
}
int SeasonalCatalogController::selectedProgressMaximum() const {
    const auto *media = selectedMedia();
    return media ? media->TotalChapters : 0;
}
double SeasonalCatalogController::selectedScoreValue() const {
    return selectedLocalMedia_ ? selectedLocalMedia_->PersonalScore : 0.0;
}
QString SeasonalCatalogController::selectedLocalPath() const {
    return selectedLocalMedia_ ? selectedLocalMedia_->LocalPath : QString();
}
QString SeasonalCatalogController::personalListErrorMessage() const { return personalListErrorMessage_; }
double SeasonalCatalogController::scoreMinimum() const { return scoreMinimum_; }
double SeasonalCatalogController::scoreMaximum() const { return scoreMaximum_; }
double SeasonalCatalogController::scoreStep() const { return scoreStep_; }

void SeasonalCatalogController::ConfigureCoverQuality(const CoverQuality quality) {
    if (coverQuality_ == quality) return;
    coverQuality_ = quality;
    mediaModel_.configurePresentation(coverQuality_, preferredTitleKey_);
    emit selectionChanged();
}

void SeasonalCatalogController::ConfigurePreferredTitle(QString key) {
    key = NormalizePreferredTitleKey(std::move(key));
    if (preferredTitleKey_ == key) return;
    preferredTitleKey_ = std::move(key);
    mediaModel_.configurePresentation(coverQuality_, preferredTitleKey_);
    emit selectionChanged();
}

void SeasonalCatalogController::ConfigureIncludeAdultContent(const bool enabled) {
    if (coordinator_) coordinator_->SetIncludeAdultContent(enabled);
}

void SeasonalCatalogController::ConfigureScoreScale(const double minimum, const double maximum,
                                                    const double step) {
    if (maximum <= minimum || step <= 0.0) return;
    if (qFuzzyCompare(scoreMinimum_, minimum) && qFuzzyCompare(scoreMaximum_, maximum)
        && qFuzzyCompare(scoreStep_, step)) return;
    scoreMinimum_ = minimum;
    scoreMaximum_ = maximum;
    scoreStep_ = step;
    emit editingOptionsChanged();
}

void SeasonalCatalogController::SetYear(const int year) { if (coordinator_) coordinator_->SetYear(year); }
void SeasonalCatalogController::SetSeason(const QString &seasonKey) { if (coordinator_) coordinator_->SetSeason(seasonKey); }
void SeasonalCatalogController::Retry() { if (coordinator_) coordinator_->Retry(); }
void SeasonalCatalogController::LoadNextPage() { if (coordinator_) coordinator_->LoadNextPage(); }

void SeasonalCatalogController::SelectMedia(const int mediaId) {
    const auto match = std::find_if(media_.cbegin(), media_.cend(), [mediaId](const Media &media) { return media.Id == mediaId; });
    if (match == media_.cend()) return;
    selectedMediaId_ = mediaId;
    refreshPersonalListMembership();
    emit selectionChanged();
}

bool SeasonalCatalogController::SaveSelectedToPersonalList(const int progress, const QString &statusKey,
                                                            const double score, const QString &path,
                                                            const QStringList &alternativeNames) {
    const Media *catalogMedia = selectedMedia();
    if (!catalogMedia) {
        personalListErrorMessage_ = tr("Selecione uma mídia antes de salvar.");
        emit personalListChanged();
        return false;
    }
    const UserListStatus status = personalListStatusFromKey(statusKey);
    if (!selectedLocalMedia_ && status == UserListStatus::Unknown) {
        personalListErrorMessage_ = tr("Selecione uma lista antes de salvar.");
        emit personalListChanged();
        return false;
    }
    if (!personalLists_) {
        personalListErrorMessage_ = tr("A lista local não está disponível.");
        emit personalListChanged();
        return false;
    }

    Media saved;
    bool created = false;
    QString error;
    const UserListStatus requestedStatus = status == UserListStatus::Unknown && selectedLocalMedia_
        ? selectedLocalMedia_->ListStatus : status;
    PersonalListMediaEdit edit{qMax(0, progress), qRound(score), path.trimmed(), alternativeNames,
                               requestedStatus};
    if (!personalLists_->save(*catalogMedia, edit, saved, created, error)) {
        personalListErrorMessage_ = std::move(error);
        emit personalListChanged();
        return false;
    }

    selectedLocalMedia_ = std::move(saved);
    personalListErrorMessage_.clear();
    emit personalListChanged();
    emit selectionChanged();
    if (created) emit personalListSaved();
    return true;
}

void SeasonalCatalogController::synchronizeFromCoordinator() {
    if (!coordinator_) return;
    media_ = coordinator_->media();
    mediaModel_.setMedia(media_);
    if (!selectedMedia()) clearSelection();
    emit filtersChanged();
    emit stateChanged();
}

void SeasonalCatalogController::clearSelection() {
    if (selectedMediaId_ == 0 && !selectedLocalMedia_ && personalListErrorMessage_.isEmpty()) return;
    selectedMediaId_ = 0;
    selectedLocalMedia_.reset();
    personalListErrorMessage_.clear();
    emit selectionChanged();
    emit personalListChanged();
}

void SeasonalCatalogController::refreshPersonalListMembership() {
    selectedLocalMedia_.reset();
    personalListErrorMessage_.clear();
    const Media *catalogMedia = selectedMedia();
    if (!catalogMedia || !personalLists_) {
        emit personalListChanged();
        return;
    }

    Media localMedia;
    bool found = false;
    QString error;
    if (personalLists_->find(catalogMedia->Id, localMedia, found, error) && found) {
        selectedLocalMedia_ = std::move(localMedia);
    } else if (!error.isEmpty()) {
        personalListErrorMessage_ = std::move(error);
    }
    emit personalListChanged();
}

const Media *SeasonalCatalogController::selectedMedia() const {
    if (selectedMediaId_ == 0) return nullptr;
    const auto match = std::find_if(media_.cbegin(), media_.cend(), [this](const Media &media) { return media.Id == selectedMediaId_; });
    return match == media_.cend() ? nullptr : &*match;
}
