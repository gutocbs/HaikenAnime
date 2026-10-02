#ifndef HAIKENANIME_SEASONALCATALOGCOORDINATOR_H
#define HAIKENANIME_SEASONALCATALOGCOORDINATOR_H

#include <QObject>
#include <QList>
#include <QVariantList>

#include <functional>
#include <optional>

#include "../application/catalog/SeasonalCatalogTypes.h"
#include "../application/media/MediaPage.h"

class ISeasonalCatalogDataSource;
class AsyncLogger;

/** Coordinates explicit seasonal requests, page bounds, and result generations. */
class SeasonalCatalogCoordinator final : public QObject {
    Q_OBJECT
public:
    explicit SeasonalCatalogCoordinator(ISeasonalCatalogDataSource &dataSource,
                                        int perPage = kSeasonalCatalogMaximumPageSize,
                                        QObject *parent = nullptr,
                                        SeasonalCatalogCachePolicy cachePolicy = {},
                                        std::function<qint64()> clock = {},
                                        std::function<void(qint64)> delay = {});

    void setLogger(AsyncLogger *logger);

    void SetYear(int year);
    void SetSeason(QString seasonKey);
    void SetIncludeAdultContent(bool enabled);
    void LoadNextPage();
    void Retry();

    [[nodiscard]] int year() const;
    [[nodiscard]] QString seasonKey() const;
    [[nodiscard]] bool includeAdultContent() const;
    [[nodiscard]] SeasonalCatalogRequest request() const;
    [[nodiscard]] SeasonalCatalogState state() const;
    [[nodiscard]] const QList<Media> &media() const;
    [[nodiscard]] QString error() const;
    [[nodiscard]] bool canLoadNextPage() const;
    [[nodiscard]] QVariantList availableYearOptions() const;
    [[nodiscard]] QVariantList availableSeasonOptions() const;

signals:
    void changed();

private:
    struct CachedPage final {
        SeasonalCatalogRequest request;
        MediaPage response;
        qint64 cachedAtMs = 0;
    };

    struct PendingFetch final {
        SeasonalCatalogRequest request;
        bool append = false;
    };

    void startFirstPageIfReady();
    void fetchPage(int page, bool append);
    void resetResult();
    [[nodiscard]] bool readCachedPage(const SeasonalCatalogRequest &request, MediaPage &response);
    void cachePage(const SeasonalCatalogRequest &request, const MediaPage &response);
    void removeCachedPage(const SeasonalCatalogRequest &request);
    void discardExpiredCacheEntries();
    void enforceMinimumRequestInterval();
    void startPendingFetchIfCurrent();

    ISeasonalCatalogDataSource &dataSource_;
    int year_ = 0;
    QString seasonKey_;
    bool includeAdultContent_ = false;
    int perPage_;
    SeasonalCatalogState state_ = SeasonalCatalogState::Idle;
    QList<Media> media_;
    QString error_;
    int currentPage_ = 0;
    int totalPages_ = 0;
    bool hasNextPage_ = false;
    QVariantList availableYearOptions_ = DefaultSeasonalCatalogYearOptions();
    QVariantList availableSeasonOptions_ = DefaultSeasonalCatalogSeasonOptions();
    quint64 generation_ = 0;
    SeasonalCatalogCachePolicy cachePolicy_;
    std::function<qint64()> clock_;
    std::function<void(qint64)> delay_;
    QList<CachedPage> cache_;
    std::optional<qint64> lastNetworkRequestAtMs_;
    bool requestInFlight_ = false;
    SeasonalCatalogRequest inFlightRequest_;
    std::optional<PendingFetch> pendingFetch_;
    AsyncLogger *logger_ = nullptr;
};

#endif // HAIKENANIME_SEASONALCATALOGCOORDINATOR_H
