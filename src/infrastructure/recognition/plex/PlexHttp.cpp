#include "PlexHttp.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <utility>

QtPlexHttp::QtPlexHttp(QNetworkAccessManager &network, const int timeoutMs) : network_(network), timeoutMs_(timeoutMs) {}

void QtPlexHttp::send(const PlexHttpRequest &request, std::function<void(const PlexHttpResponse &)> completion) {
    QNetworkRequest networkRequest(request.url);
    for (const auto &[name, value] : request.headers) networkRequest.setRawHeader(name, value);
    // Every external call has a timeout; an expired transfer reports status 0 (unavailable).
    networkRequest.setTransferTimeout(timeoutMs_);
    auto *reply = network_.sendCustomRequest(networkRequest, request.method, QByteArray());
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, completion = std::move(completion)] {
        PlexHttpResponse response;
        response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        response.body = reply->readAll();
        reply->deleteLater();
        completion(response);
    });
}
