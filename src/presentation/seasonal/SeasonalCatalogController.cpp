#include "SeasonalCatalogController.h"

#include "../../app/SeasonalCatalogCoordinator.h"
#include "../../application/covers/CoverSourceResolver.h"

#include <QCoreApplication>
#include <QDate>
#include <QSet>

#include <algorithm>
#include <utility>

namespace {
QVariantMap option(const QString &key, const QString &label) {
    return {{QStringLiteral("key"), key}, {QStringLiteral("label"), label}};
}

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

QString seasonLabel(const Media &media) {
    const auto season = media.Season == QStringLiteral("WINTER") ? QCoreApplication::translate("SeasonalCatalogController", "Inverno")
        : media.Season == QStringLiteral("SPRING") ? QCoreApplication::translate("SeasonalCatalogController", "Primavera")
        : media.Season == QStringLiteral("SUMMER") ? QCoreApplication::translate("SeasonalCatalogController", "Verão")
        : media.Season == QStringLiteral("FALL") ? QCoreApplication::translate("SeasonalCatalogController", "Outono")
        : media.Season.simplified();
    return media.SeasonYear && !season.isEmpty() ? QStringLiteral("%1 de %2").arg(season).arg(*media.SeasonYear)
        : media.SeasonYear ? QString::number(*media.SeasonYear) : season;
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
    case ProgressRole: return seasonLabel(media);
    case ScoreRole: return media.AverageScore > 0 ? QStringLiteral("AniList %1").arg(media.AverageScore) : QStringLiteral("AniList —");
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
                                                     QObject *parent)
    : QObject(parent), coordinator_(coordinator), mediaModel_(this), coverQuality_(quality),
      preferredTitleKey_(NormalizePreferredTitleKey(std::move(preferredTitleKey))) {
    mediaModel_.configurePresentation(coverQuality_, preferredTitleKey_);
    if (coordinator_) {
        connect(coordinator_, &SeasonalCatalogCoordinator::changed, this,
                &SeasonalCatalogController::synchronizeFromCoordinator);
        synchronizeFromCoordinator();
    }
}

SeasonalCatalogMediaModel *SeasonalCatalogController::mediaModel() { return &mediaModel_; }

QVariantList SeasonalCatalogController::availableYearOptions() const {
    QVariantList options;
    const int currentYear = QDate::currentDate().year();
    for (int year = currentYear + 1; year >= currentYear - 10; --year) {
        options.append(option(QString::number(year), QString::number(year)));
    }
    return options;
}

QVariantList SeasonalCatalogController::availableSeasonOptions() const {
    return {option(QStringLiteral("WINTER"), QCoreApplication::translate("SeasonalCatalogController", "Inverno")),
            option(QStringLiteral("SPRING"), QCoreApplication::translate("SeasonalCatalogController", "Primavera")),
            option(QStringLiteral("SUMMER"), QCoreApplication::translate("SeasonalCatalogController", "Verão")),
            option(QStringLiteral("FALL"), QCoreApplication::translate("SeasonalCatalogController", "Outono"))};
}

int SeasonalCatalogController::selectedYear() const { return coordinator_ ? coordinator_->year() : 0; }
QString SeasonalCatalogController::selectedSeasonKey() const { return coordinator_ ? coordinator_->seasonKey() : QString(); }
QString SeasonalCatalogController::state() const { return coordinator_ ? stateName(coordinator_->state()) : QStringLiteral("error"); }
QString SeasonalCatalogController::errorMessage() const { return coordinator_ ? coordinator_->error() : QStringLiteral("Seasonal catalog is unavailable."); }
bool SeasonalCatalogController::canLoadNextPage() const { return coordinator_ && coordinator_->canLoadNextPage(); }
bool SeasonalCatalogController::hasSelection() const { return selectedMedia() != nullptr; }
int SeasonalCatalogController::selectedMediaId() const { return hasSelection() ? selectedMediaId_ : 0; }
QString SeasonalCatalogController::selectedTitle() const { const auto *media = selectedMedia(); return media ? ResolveMediaTitle(*media, preferredTitleKey_) : QString(); }
QString SeasonalCatalogController::selectedSynopsis() const { const auto *media = selectedMedia(); return media ? media->Synopsis : QString(); }
QString SeasonalCatalogController::selectedTypeLabel() const { const auto *media = selectedMedia(); return media ? mediaTypeLabel(media->Type) : QString(); }
QString SeasonalCatalogController::selectedStatusLabel() const { const auto *media = selectedMedia(); return media ? mediaStatusLabel(media->Status) : QString(); }
QString SeasonalCatalogController::selectedProgress() const { return hasSelection() ? QStringLiteral("—") : QString(); }
QString SeasonalCatalogController::selectedScore() const { return hasSelection() ? QStringLiteral("—") : QString(); }
QString SeasonalCatalogController::selectedAverageScore() const { const auto *media = selectedMedia(); return media && media->AverageScore > 0 ? QString::number(media->AverageScore) : hasSelection() ? QStringLiteral("—") : QString(); }
QString SeasonalCatalogController::selectedSeasonLabel() const { const auto *media = selectedMedia(); return media ? seasonLabel(*media) : QString(); }
QString SeasonalCatalogController::selectedNextAiringLabel() const { return {}; }
QVariantList SeasonalCatalogController::selectedMediaLinks() const {
    const auto *media = selectedMedia();
    if (!media) return {};
    QVariantList links;
    if (!media->AniListUrl.isEmpty()) {
        links.append(QVariantMap{{QStringLiteral("site"), QStringLiteral("AniList")},
                                 {QStringLiteral("url"), media->AniListUrl}});
    }
    for (const auto &link : media->ExternalLinks) {
        if (!link.Site.isEmpty() && !link.Url.isEmpty()) {
            links.append(QVariantMap{{QStringLiteral("site"), link.Site},
                                     {QStringLiteral("url"), link.Url}});
        }
    }
    return links;
}
QString SeasonalCatalogController::selectedCoverSource() const { const auto *media = selectedMedia(); return media ? ResolveCoverSource(*media, coverQuality_) : QStringLiteral("qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg"); }
QStringList SeasonalCatalogController::selectedAlternativeNames() const { const auto *media = selectedMedia(); return media ? media->AlternativeNames : QStringList{}; }

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

void SeasonalCatalogController::SetYear(const int year) { if (coordinator_) coordinator_->SetYear(year); }
void SeasonalCatalogController::SetSeason(const QString &seasonKey) { if (coordinator_) coordinator_->SetSeason(seasonKey); }
void SeasonalCatalogController::Retry() { if (coordinator_) coordinator_->Retry(); }
void SeasonalCatalogController::LoadNextPage() { if (coordinator_) coordinator_->LoadNextPage(); }

void SeasonalCatalogController::SelectMedia(const int mediaId) {
    const auto match = std::find_if(media_.cbegin(), media_.cend(), [mediaId](const Media &media) { return media.Id == mediaId; });
    if (match == media_.cend()) return;
    selectedMediaId_ = mediaId;
    emit selectionChanged();
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
    if (selectedMediaId_ == 0) return;
    selectedMediaId_ = 0;
    emit selectionChanged();
}

const Media *SeasonalCatalogController::selectedMedia() const {
    if (selectedMediaId_ == 0) return nullptr;
    const auto match = std::find_if(media_.cbegin(), media_.cend(), [this](const Media &media) { return media.Id == selectedMediaId_; });
    return match == media_.cend() ? nullptr : &*match;
}
