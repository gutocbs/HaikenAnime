#ifndef HAIKENANIME_SEASONALCATALOGCOORDINATOR_H
#define HAIKENANIME_SEASONALCATALOGCOORDINATOR_H

#include <QObject>
#include <QVariantList>

#include "../application/catalog/SeasonalCatalogTypes.h"
#include "../application/media/MediaPage.h"

class ISeasonalCatalogDataSource;

/** Coordinates explicit seasonal requests, page bounds, and result generations. */
class SeasonalCatalogCoordinator final : public QObject {
    Q_OBJECT
public:
    explicit SeasonalCatalogCoordinator(ISeasonalCatalogDataSource &dataSource,
                                        int perPage = kSeasonalCatalogMaximumPageSize,
                                        QObject *parent = nullptr);

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
    void startFirstPageIfReady();
    void fetchPage(int page, bool append);
    void resetResult();

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
};

#endif // HAIKENANIME_SEASONALCATALOGCOORDINATOR_H
