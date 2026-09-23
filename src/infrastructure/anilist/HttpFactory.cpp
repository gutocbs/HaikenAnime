#include "HttpFactory.h"

#include <QNetworkAccessManager>

QNetworkAccessManager *HttpFactory::createNetworkAccessManager(QObject *parent) {
    return new QNetworkAccessManager(parent);
}
