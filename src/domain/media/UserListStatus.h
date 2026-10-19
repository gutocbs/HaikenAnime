#ifndef HAIKENANIME_USERLISTSTATUS_H
#define HAIKENANIME_USERLISTSTATUS_H

#include <QString>
#include <QStringList>

enum class UserListStatus {
    Unknown = -1,
    Current = 0,
    Planning = 1,
    OnHold = 2,
    Dropped = 3,
    Completed = 4
};

/** Stable key used by preferences, list filters and QML; "unknown" when the media is in no list. */
inline QString UserListStatusKey(const UserListStatus status) {
    switch (status) {
    case UserListStatus::Current: return QStringLiteral("current");
    case UserListStatus::Planning: return QStringLiteral("planning");
    case UserListStatus::OnHold: return QStringLiteral("on_hold");
    case UserListStatus::Dropped: return QStringLiteral("dropped");
    case UserListStatus::Completed: return QStringLiteral("completed");
    case UserListStatus::Unknown:
    default: return QStringLiteral("unknown");
    }
}

/** Inverse of UserListStatusKey; any other key maps to Unknown. */
inline UserListStatus UserListStatusFromKey(const QString &key) {
    if (key == QStringLiteral("current")) return UserListStatus::Current;
    if (key == QStringLiteral("planning")) return UserListStatus::Planning;
    if (key == QStringLiteral("on_hold")) return UserListStatus::OnHold;
    if (key == QStringLiteral("dropped")) return UserListStatus::Dropped;
    if (key == QStringLiteral("completed")) return UserListStatus::Completed;
    return UserListStatus::Unknown;
}

/** Keys of every personal list, in display order. */
inline QStringList UserListKeys() {
    return {QStringLiteral("current"), QStringLiteral("planning"), QStringLiteral("on_hold"),
            QStringLiteral("dropped"), QStringLiteral("completed")};
}

#endif // HAIKENANIME_USERLISTSTATUS_H
