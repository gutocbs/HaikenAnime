#ifndef HAIKENANIME_SEASONALCATALOGTYPES_H
#define HAIKENANIME_SEASONALCATALOGTYPES_H

#include <QString>
#include <QDate>
#include <QVariantList>

inline constexpr int kSeasonalCatalogMaximumPageSize = 50;

/** Bounds in-memory seasonal pages and spacing between public AniList requests. */
struct SeasonalCatalogCachePolicy final {
    int maximumEntries = 24;
    qint64 timeToLiveMs = 5 * 60 * 1000;
    qint64 minimumRequestIntervalMs = 1000;
};

inline QVariantList DefaultSeasonalCatalogYearOptions(const int referenceYear = QDate::currentDate().year()) {
    QVariantList options;
    for (int year = referenceYear + 1; year >= referenceYear - 10; --year) {
        options.append(QVariantMap{{QStringLiteral("key"), QString::number(year)},
                                   {QStringLiteral("label"), QString::number(year)}});
    }
    return options;
}

inline QVariantList DefaultSeasonalCatalogSeasonOptions() {
    return {QVariantMap{{QStringLiteral("key"), QStringLiteral("WINTER")},
                        {QStringLiteral("label"), QStringLiteral("Inverno")}},
            QVariantMap{{QStringLiteral("key"), QStringLiteral("SPRING")},
                        {QStringLiteral("label"), QStringLiteral("Primavera")}},
            QVariantMap{{QStringLiteral("key"), QStringLiteral("SUMMER")},
                        {QStringLiteral("label"), QStringLiteral("Verão")}},
            QVariantMap{{QStringLiteral("key"), QStringLiteral("FALL")},
                        {QStringLiteral("label"), QStringLiteral("Outono")}}};
}

/** Identifies one explicit, bounded AniList seasonal catalog page. */
struct SeasonalCatalogRequest final {
    int year = 0;
    QString seasonKey;
    int page = 1;
    int perPage = kSeasonalCatalogMaximumPageSize;
    bool includeAdultContent = false;

    [[nodiscard]] bool isValid() const {
        return year >= 1 && year <= 9999 && page >= 1 && perPage >= 1
            && perPage <= kSeasonalCatalogMaximumPageSize && isValidSeasonKey(seasonKey);
    }

    [[nodiscard]] static bool isValidSeasonKey(const QString &key) {
        return key == QStringLiteral("WINTER") || key == QStringLiteral("SPRING")
            || key == QStringLiteral("SUMMER") || key == QStringLiteral("FALL");
    }

    friend bool operator==(const SeasonalCatalogRequest &, const SeasonalCatalogRequest &) = default;
};

enum class SeasonalCatalogState { Idle, Loading, Populated, Empty, Error };

#endif // HAIKENANIME_SEASONALCATALOGTYPES_H
