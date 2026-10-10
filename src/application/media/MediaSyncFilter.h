#ifndef HAIKENANIME_MEDIASYNCFILTER_H
#define HAIKENANIME_MEDIASYNCFILTER_H

#include <QString>
#include <QStringList>

#include "../scheduling/SyncTaskTypes.h"

/** Selects and paginates media requested from an application data source. */
struct MediaSyncFilter {
    SyncPartition partition = SyncPartition::UserList;
    QString username;
    QString type;
    QString status;
    QString list;
    QStringList acceptedListStatuses;
    int startingPage = 1;
    int perPage = 50;

    [[nodiscard]] bool isEmpty() const {
        return username.isEmpty() && type.isEmpty() && status.isEmpty() && list.isEmpty()
            && acceptedListStatuses.isEmpty();
    }
};

inline QStringList AniListStatusesForEnabledUserLists(const QStringList &keys) {
    QStringList statuses;
    for (const auto &key : keys) {
        if (key == QStringLiteral("current")) statuses.append(QStringLiteral("CURRENT"));
        else if (key == QStringLiteral("planning")) statuses.append(QStringLiteral("PLANNING"));
        else if (key == QStringLiteral("on_hold")) statuses.append(QStringLiteral("PAUSED"));
        else if (key == QStringLiteral("dropped")) statuses.append(QStringLiteral("DROPPED"));
        else if (key == QStringLiteral("completed")) statuses.append(QStringLiteral("COMPLETED"));
    }
    return statuses;
}

#endif
