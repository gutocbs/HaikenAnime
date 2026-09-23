#ifndef HAIKENANIME_MEDIASYNCFILTER_H
#define HAIKENANIME_MEDIASYNCFILTER_H

#include <QString>

struct MediaSyncFilter {
    QString username;
    QString type;
    QString status;
    QString list;
    int startingPage = 1;
    int perPage = 50;

    [[nodiscard]] bool isEmpty() const {
        return username.isEmpty() && type.isEmpty() && status.isEmpty() && list.isEmpty();
    }
};

#endif
