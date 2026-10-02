#include "SeasonalCatalogCoordinator.h"

#include "../application/catalog/ISeasonalCatalogDataSource.h"

#include <QSet>

#include <utility>

SeasonalCatalogCoordinator::SeasonalCatalogCoordinator(ISeasonalCatalogDataSource &dataSource,
                                                       const int perPage, QObject *parent)
    : QObject(parent), dataSource_(dataSource),
      perPage_(qBound(1, perPage, kSeasonalCatalogMaximumPageSize)) {
}

void SeasonalCatalogCoordinator::SetYear(const int year) {
    if (year_ == year) return;
    year_ = year;
    resetResult();
    startFirstPageIfReady();
}

void SeasonalCatalogCoordinator::SetSeason(QString seasonKey) {
    if (seasonKey_ == seasonKey) return;
    seasonKey_ = std::move(seasonKey);
    resetResult();
    startFirstPageIfReady();
}

void SeasonalCatalogCoordinator::LoadNextPage() {
    if (!canLoadNextPage()) return;
    fetchPage(currentPage_ + 1, true);
}

void SeasonalCatalogCoordinator::Retry() {
    if (!request().isValid()) return;
    fetchPage(1, false);
}

int SeasonalCatalogCoordinator::year() const { return year_; }
QString SeasonalCatalogCoordinator::seasonKey() const { return seasonKey_; }
SeasonalCatalogRequest SeasonalCatalogCoordinator::request() const { return {year_, seasonKey_, 1, perPage_}; }
SeasonalCatalogState SeasonalCatalogCoordinator::state() const { return state_; }
const QList<Media> &SeasonalCatalogCoordinator::media() const { return media_; }
QString SeasonalCatalogCoordinator::error() const { return error_; }

bool SeasonalCatalogCoordinator::canLoadNextPage() const {
    return request().isValid() && state_ != SeasonalCatalogState::Loading && hasNextPage_
        && currentPage_ >= 1 && currentPage_ < totalPages_;
}

void SeasonalCatalogCoordinator::startFirstPageIfReady() {
    if (!request().isValid()) {
        emit changed();
        return;
    }
    fetchPage(1, false);
}

void SeasonalCatalogCoordinator::fetchPage(const int page, const bool append) {
    SeasonalCatalogRequest pageRequest{year_, seasonKey_, page, perPage_};
    if (!pageRequest.isValid()) return;

    const quint64 requestGeneration = ++generation_;
    state_ = SeasonalCatalogState::Loading;
    error_.clear();
    emit changed();

    MediaPage response;
    QString requestError;
    const bool succeeded = dataSource_.Fetch(pageRequest, response, requestError);
    if (requestGeneration != generation_) return;

    if (!succeeded) {
        state_ = SeasonalCatalogState::Error;
        error_ = requestError.isEmpty() ? QStringLiteral("AniList seasonal catalog request failed.")
                                       : requestError;
        emit changed();
        return;
    }
    if (response.currentPage != pageRequest.page || response.totalPages < response.currentPage) {
        state_ = SeasonalCatalogState::Error;
        error_ = QStringLiteral("AniList seasonal catalog returned invalid pagination metadata.");
        emit changed();
        return;
    }

    QList<Media> merged = append ? media_ : QList<Media>{};
    QSet<int> seenMediaIds;
    for (const auto &entry : merged) seenMediaIds.insert(entry.Id);
    for (const auto &entry : response.media) {
        if (!seenMediaIds.contains(entry.Id)) {
            seenMediaIds.insert(entry.Id);
            merged.append(entry);
        }
    }

    media_ = std::move(merged);
    currentPage_ = response.currentPage;
    totalPages_ = response.totalPages;
    hasNextPage_ = response.hasNextPage && currentPage_ < totalPages_;
    state_ = media_.isEmpty() ? SeasonalCatalogState::Empty : SeasonalCatalogState::Populated;
    emit changed();
}

void SeasonalCatalogCoordinator::resetResult() {
    ++generation_;
    media_.clear();
    error_.clear();
    currentPage_ = 0;
    totalPages_ = 0;
    hasNextPage_ = false;
    state_ = SeasonalCatalogState::Idle;
}
