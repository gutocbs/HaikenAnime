#ifndef HAIKENANIME_HTTPFACTORY_H
#define HAIKENANIME_HTTPFACTORY_H

#include <QObject>

class QNetworkAccessManager;

/** Creates network objects with the ownership required by their execution thread. */
class HttpFactory final {
public:
    /** Creates a network manager owned by parent and intended for the parent's thread. */
    [[nodiscard]] static QNetworkAccessManager *createNetworkAccessManager(QObject *parent);
};

#endif // HAIKENANIME_HTTPFACTORY_H
