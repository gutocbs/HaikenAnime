#include "MediaEnumFormatter.h"

QString MediaEnumFormatter::TypeToString(const MediaType type) {
    switch (type) {
    case Anime:
        return QStringLiteral("Anime");
    case Manga:
        return QStringLiteral("Manga");
    case Novel:
        return QStringLiteral("Novel");
    }
    return QStringLiteral("Unknown");
}

QString MediaEnumFormatter::StatusToString(const MediaStatus status) {
    switch (status) {
    case NotReleased:
        return QStringLiteral("NotReleased");
    case Releasing:
        return QStringLiteral("Releasing");
    case Released:
        return QStringLiteral("Released");
    }
    return QStringLiteral("Unknown");
}
