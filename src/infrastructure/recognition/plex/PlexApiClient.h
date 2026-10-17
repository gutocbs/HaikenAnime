#ifndef HAIKENANIME_PLEXAPICLIENT_H
#define HAIKENANIME_PLEXAPICLIENT_H

#include <QDate>
#include <QList>
#include <QString>
#include <QUrl>

#include <functional>
#include <optional>

#include "PlexHttp.h"

/// Identifies this HaikenAnime installation to plex.tv and media servers.
struct PlexClientIdentity final {
    /// Persistent, unique per installation.
    QString clientIdentifier;
    QString product = QStringLiteral("HaikenAnime");
};

enum class PlexApiError { None, Unauthorized, Unavailable, InvalidResponse };

struct PlexConnection final {
    QUrl uri;
    bool local = false;
    bool relay = false;
};

struct PlexServer final {
    QString name;
    QString clientIdentifier;
    /// Server-specific token from plex.tv; kept in memory only.
    QString accessToken;
    bool owned = false;
    QList<PlexConnection> connections;

    /// Local direct connection first, then remote direct, relay last.
    [[nodiscard]] std::optional<QUrl> preferredUri() const;
};

struct PlexAccount final {
    QString id;
    QString name;
};

struct PlexSession final {
    QString sessionKey;
    QString type;
    QString showTitle;
    QString episodeTitle;
    QString summary;
    std::optional<QDate> airDate;
    std::optional<int> season;
    std::optional<int> episode;
    qint64 viewOffsetMs = 0;
    qint64 durationMs = 0;
    QString userId;
    QString userName;
    QString playerId;
    QString playerTitle;
    /// playing, paused or buffering.
    QString state;
};

/// plex.tv resources and Plex Media Server endpoints. See
/// docs/superpowers/references/2026-10-17-mpv-plex-contratos.md.
class PlexApiClient final {
public:
    PlexApiClient(IPlexHttp &http, PlexClientIdentity identity);

    void discoverServers(const QString &accountToken,
                         std::function<void(PlexApiError, const QList<PlexServer> &)> completion);
    void listUsers(const PlexServer &server, std::function<void(PlexApiError, const QList<PlexAccount> &)> completion);
    void listSessions(const PlexServer &server,
                      std::function<void(PlexApiError, const QList<PlexSession> &)> completion);

    [[nodiscard]] QList<QPair<QByteArray, QByteArray>> headers(const QString &token) const;

private:
    IPlexHttp &http_;
    PlexClientIdentity identity_;
};

#endif // HAIKENANIME_PLEXAPICLIENT_H
