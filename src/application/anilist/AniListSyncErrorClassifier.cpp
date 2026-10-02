#include "AniListSyncErrorClassifier.h"

AniListSyncErrorCategory AniListSyncErrorClassifier::Classify(const QString &error) {
    const auto value = error.toLower();
    if (value.contains(QStringLiteral("timeout")) || value.contains(QStringLiteral("timed out"))) return AniListSyncErrorCategory::Timeout;
    if (value.contains(QStringLiteral("credential")) || value.contains(QStringLiteral("unauthorized")) || value.contains(QStringLiteral("authentication"))) return AniListSyncErrorCategory::Authentication;
    if (value.contains(QStringLiteral("forbidden")) || value.contains(QStringLiteral("permission"))) return AniListSyncErrorCategory::Authorization;
    if (value.contains(QStringLiteral("rate limit")) || value.contains(QStringLiteral("429"))) return AniListSyncErrorCategory::RateLimit;
    if (value.contains(QStringLiteral("graphql")) || value.contains(QStringLiteral("mutation"))) return AniListSyncErrorCategory::GraphQl;
    if (value.contains(QStringLiteral("database")) || value.contains(QStringLiteral("sqlite")) || value.contains(QStringLiteral("persist"))) return AniListSyncErrorCategory::Persistence;
    if (value.contains(QStringLiteral("json")) || value.contains(QStringLiteral("fixture")) || value.contains(QStringLiteral("invalid data"))) return AniListSyncErrorCategory::InvalidData;
    if (value.contains(QStringLiteral("network")) || value.contains(QStringLiteral("connection")) || value.contains(QStringLiteral("socket"))) return AniListSyncErrorCategory::Network;
    if (value.contains(QStringLiteral("cancel"))) return AniListSyncErrorCategory::Cancelled;
    return error.isEmpty() ? AniListSyncErrorCategory::None : AniListSyncErrorCategory::Unknown;
}
