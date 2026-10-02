#include "SeasonalCatalogCoordinator.h"

#include "../application/catalog/ISeasonalCatalogDataSource.h"
#include "../infrastructure/logging/AsyncLogger.h"

#include <QDateTime>
#include <QSet>
#include <QThread>

#include <utility>

SeasonalCatalogCoordinator::SeasonalCatalogCoordinator(ISeasonalCatalogDataSource &dataSource,
                                                       const int perPage, QObject *parent,
                                                       SeasonalCatalogCachePolicy cachePolicy,
                                                       std::function<qint64()> clock,
                                                       std::function<void(qint64)> delay)
    : QObject(parent), dataSource_(dataSource),
      perPage_(qBound(1, perPage, kSeasonalCatalogMaximumPageSize)),
      cachePolicy_{qMax(0, cachePolicy.maximumEntries), qMax<qint64>(0, cachePolicy.timeToLiveMs),
                   qMax<qint64>(0, cachePolicy.minimumRequestIntervalMs)},
      clock_(std::move(clock)), delay_(std::move(delay)) {
    if (!clock_) clock_ = [] { return QDateTime::currentMSecsSinceEpoch(); };
    if (!delay_) delay_ = [](const qint64 milliseconds) {
        if (milliseconds > 0) QThread::msleep(static_cast<unsigned long>(milliseconds));
    };
}

void SeasonalCatalogCoordinator::setLogger(AsyncLogger *logger) { logger_ = logger; }

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

void SeasonalCatalogCoordinator::SetIncludeAdultContent(const bool enabled) {
    if (includeAdultContent_ == enabled) return;
    includeAdultContent_ = enabled;
    resetResult();
    startFirstPageIfReady();
}

void SeasonalCatalogCoordinator::LoadNextPage() {
    if (!canLoadNextPage()) return;
    fetchPage(currentPage_ + 1, true);
}

void SeasonalCatalogCoordinator::Retry() {
    if (!request().isValid()) return;
    removeCachedPage(request());
    fetchPage(1, false);
}

int SeasonalCatalogCoordinator::year() const { return year_; }
QString SeasonalCatalogCoordinator::seasonKey() const { return seasonKey_; }
bool SeasonalCatalogCoordinator::includeAdultContent() const { return includeAdultContent_; }
SeasonalCatalogRequest SeasonalCatalogCoordinator::request() const { return {year_, seasonKey_, 1, perPage_, includeAdultContent_}; }
SeasonalCatalogState SeasonalCatalogCoordinator::state() const { return state_; }
const QList<Media> &SeasonalCatalogCoordinator::media() const { return media_; }
QString SeasonalCatalogCoordinator::error() const { return error_; }

bool SeasonalCatalogCoordinator::canLoadNextPage() const {
    return request().isValid() && state_ != SeasonalCatalogState::Loading && hasNextPage_
        && currentPage_ >= 1 && currentPage_ < totalPages_;
}

QVariantList SeasonalCatalogCoordinator::availableYearOptions() const { return availableYearOptions_; }
QVariantList SeasonalCatalogCoordinator::availableSeasonOptions() const { return availableSeasonOptions_; }

void SeasonalCatalogCoordinator::startFirstPageIfReady() {
    if (!request().isValid()) {
        emit changed();
        return;
    }
    fetchPage(1, false);
}

void SeasonalCatalogCoordinator::fetchPage(const int page, const bool append) {
    SeasonalCatalogRequest pageRequest{year_, seasonKey_, page, perPage_, includeAdultContent_};
    if (!pageRequest.isValid()) return;

    if (requestInFlight_) {
        if (pageRequest != inFlightRequest_) pendingFetch_ = PendingFetch{pageRequest, append};
        return;
    }

    const quint64 requestGeneration = ++generation_;
    state_ = SeasonalCatalogState::Loading;
    error_.clear();
    emit changed();

    MediaPage response;
    QString requestError;
    const bool cacheHit = readCachedPage(pageRequest, response);
    bool succeeded = cacheHit;
    if (!cacheHit) {
        enforceMinimumRequestInterval();
        requestInFlight_ = true;
        inFlightRequest_ = pageRequest;
        succeeded = dataSource_.Fetch(pageRequest, response, requestError);
        requestInFlight_ = false;
        inFlightRequest_ = {};
    }
    const SeasonalCatalogRequest currentRequest{year_, seasonKey_, page, perPage_, includeAdultContent_};
    if (requestGeneration != generation_ && pageRequest != currentRequest) {
        startPendingFetchIfCurrent();
        return;
    }

    if (!succeeded) {
        state_ = SeasonalCatalogState::Error;
        error_ = requestError.isEmpty() ? QStringLiteral("AniList seasonal catalog request failed.")
                                       : requestError;
        if (logger_) logger_->error(LogCategory::Sync, error_);
        emit changed();
        startPendingFetchIfCurrent();
        return;
    }
    if (response.currentPage != pageRequest.page || response.totalPages < response.currentPage) {
        state_ = SeasonalCatalogState::Error;
        error_ = QStringLiteral("AniList seasonal catalog returned invalid pagination metadata.");
        if (logger_) logger_->error(LogCategory::Sync, error_);
        emit changed();
        startPendingFetchIfCurrent();
        return;
    }

    if (!cacheHit) cachePage(pageRequest, response);

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
    startPendingFetchIfCurrent();
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

bool SeasonalCatalogCoordinator::readCachedPage(const SeasonalCatalogRequest &request, MediaPage &response) {
    discardExpiredCacheEntries();
    for (qsizetype index = 0; index < cache_.size(); ++index) {
        if (cache_.at(index).request != request) continue;
        const auto entry = cache_.takeAt(index);
        response = entry.response;
        cache_.append(entry);
        if (logger_) logger_->info(LogCategory::Sync, QStringLiteral("Seasonal catalog cache hit."));
        return true;
    }
    return false;
}

void SeasonalCatalogCoordinator::cachePage(const SeasonalCatalogRequest &request, const MediaPage &response) {
    if (cachePolicy_.maximumEntries <= 0 || cachePolicy_.timeToLiveMs <= 0) return;
    discardExpiredCacheEntries();
    for (qsizetype index = cache_.size() - 1; index >= 0; --index) {
        if (cache_.at(index).request == request) cache_.removeAt(index);
    }
    cache_.append({request, response, clock_()});
    while (cache_.size() > cachePolicy_.maximumEntries) cache_.removeFirst();
}

void SeasonalCatalogCoordinator::removeCachedPage(const SeasonalCatalogRequest &request) {
    for (qsizetype index = cache_.size() - 1; index >= 0; --index) {
        if (cache_.at(index).request == request) cache_.removeAt(index);
    }
}

void SeasonalCatalogCoordinator::discardExpiredCacheEntries() {
    if (cachePolicy_.timeToLiveMs <= 0) {
        cache_.clear();
        return;
    }
    const auto now = clock_();
    for (qsizetype index = cache_.size() - 1; index >= 0; --index) {
        const auto age = qMax<qint64>(0, now - cache_.at(index).cachedAtMs);
        if (age >= cachePolicy_.timeToLiveMs) cache_.removeAt(index);
    }
}

void SeasonalCatalogCoordinator::enforceMinimumRequestInterval() {
    const auto now = clock_();
    if (lastNetworkRequestAtMs_) {
        const auto elapsed = now - *lastNetworkRequestAtMs_;
        const auto delayMs = qMax<qint64>(0, cachePolicy_.minimumRequestIntervalMs - elapsed);
        if (delayMs > 0) {
            if (logger_) logger_->info(LogCategory::Sync,
                                       QStringLiteral("Seasonal catalog request throttled by %1 ms.").arg(delayMs));
            delay_(delayMs);
        }
    }
    lastNetworkRequestAtMs_ = clock_();
}

void SeasonalCatalogCoordinator::startPendingFetchIfCurrent() {
    if (!pendingFetch_) return;
    const auto pending = *pendingFetch_;
    pendingFetch_.reset();
    const SeasonalCatalogRequest current{year_, seasonKey_, pending.request.page, perPage_, includeAdultContent_};
    if (pending.request == current) fetchPage(pending.request.page, pending.append);
}
