#ifndef HAIKENANIME_PLEXHTTP_H
#define HAIKENANIME_PLEXHTTP_H

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;

struct PlexHttpRequest final {
    QByteArray method = "GET";
    QUrl url;
    QList<QPair<QByteArray, QByteArray>> headers;
};

struct PlexHttpResponse final {
    /// HTTP status; 0 when no response arrived (network error or timeout).
    int status = 0;
    QByteArray body;
};

/// Minimal HTTP port so Plex flows are testable without plex.tv or a media server.
class IPlexHttp {
public:
    virtual ~IPlexHttp() = default;

    virtual void send(const PlexHttpRequest &request, std::function<void(const PlexHttpResponse &)> completion) = 0;
};

/// QNetworkAccessManager adapter with a mandatory per-request timeout.
class QtPlexHttp final : public IPlexHttp {
public:
    QtPlexHttp(QNetworkAccessManager &network, int timeoutMs);

    void send(const PlexHttpRequest &request, std::function<void(const PlexHttpResponse &)> completion) override;

private:
    QNetworkAccessManager &network_;
    int timeoutMs_;
};

#endif // HAIKENANIME_PLEXHTTP_H
