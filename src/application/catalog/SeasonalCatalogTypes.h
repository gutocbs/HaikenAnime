#ifndef HAIKENANIME_SEASONALCATALOGTYPES_H
#define HAIKENANIME_SEASONALCATALOGTYPES_H

#include <QString>

inline constexpr int kSeasonalCatalogMaximumPageSize = 50;

/** Identifies one explicit, bounded AniList seasonal catalog page. */
struct SeasonalCatalogRequest final {
    int year = 0;
    QString seasonKey;
    int page = 1;
    int perPage = kSeasonalCatalogMaximumPageSize;

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
