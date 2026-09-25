#ifndef HAIKENANIME_COVERQUALITY_H
#define HAIKENANIME_COVERQUALITY_H

#include <QMetaType>
#include <QString>

#include <optional>

enum class CoverQuality { Medium, Large, ExtraLarge };

inline QString CoverQualityName(const CoverQuality quality) {
    switch (quality) {
    case CoverQuality::Medium: return QStringLiteral("medium");
    case CoverQuality::Large: return QStringLiteral("large");
    case CoverQuality::ExtraLarge: return QStringLiteral("extraLarge");
    }
    return {};
}

inline std::optional<CoverQuality> ParseCoverQuality(const QString &value) {
    if (value == QStringLiteral("medium")) return CoverQuality::Medium;
    if (value == QStringLiteral("large")) return CoverQuality::Large;
    if (value == QStringLiteral("extraLarge")) return CoverQuality::ExtraLarge;
    return std::nullopt;
}

Q_DECLARE_METATYPE(CoverQuality)

#endif // HAIKENANIME_COVERQUALITY_H
