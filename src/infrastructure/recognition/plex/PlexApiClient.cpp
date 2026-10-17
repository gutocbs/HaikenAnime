#include "PlexApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include <utility>

namespace {
const auto ResourcesUrl = QStringLiteral("https://clients.plex.tv/api/v2/resources");

PlexApiError ErrorForStatus(const int status) {
    if (status == 401 || status == 403) return PlexApiError::Unauthorized;
    if (status >= 200 && status < 300) return PlexApiError::None;
    return PlexApiError::Unavailable;
}

QString Text(const QJsonValue &value) {
    // Plex returns some identifiers as numbers and others as strings.
    if (value.isDouble()) return QString::number(value.toInteger());
    return value.toString();
}

std::optional<int> OptionalIndex(const QJsonValue &value) {
    if (value.isDouble()) return static_cast<int>(value.toInteger());
    bool converted = false;
    const auto number = value.toString().toInt(&converted);
    return converted ? std::optional<int>(number) : std::nullopt;
}

qint64 Milliseconds(const QJsonValue &value) {
    return value.isDouble() ? value.toInteger() : value.toString().toLongLong();
}

std::optional<QJsonObject> MediaContainer(const QByteArray &body) {
    const auto document = QJsonDocument::fromJson(body);
    if (!document.isObject()) return std::nullopt;
    const auto container = document.object().value(QStringLiteral("MediaContainer"));
    if (!container.isObject()) return std::nullopt;
    return container.toObject();
}

QUrl ServerUrl(const PlexServer &server, const QString &path) {
    auto url = *server.preferredUri();
    url.setPath(url.path() + path);
    return url;
}
}

std::optional<QUrl> PlexServer::preferredUri() const {
    const auto pick = [this](const auto &accept) -> std::optional<QUrl> {
        for (const auto &connection : connections) {
            if (connection.uri.isValid() && accept(connection)) return connection.uri;
        }
        return std::nullopt;
    };
    if (auto local = pick([](const PlexConnection &c) { return c.local && !c.relay; })) return local;
    if (auto direct = pick([](const PlexConnection &c) { return !c.relay; })) return direct;
    return pick([](const PlexConnection &) { return true; });
}

PlexApiClient::PlexApiClient(IPlexHttp &http, PlexClientIdentity identity)
    : http_(http), identity_(std::move(identity)) {}

QList<QPair<QByteArray, QByteArray>> PlexApiClient::headers(const QString &token) const {
    QList<QPair<QByteArray, QByteArray>> result{{"Accept", "application/json"},
                                                {"X-Plex-Product", identity_.product.toUtf8()},
                                                {"X-Plex-Client-Identifier", identity_.clientIdentifier.toUtf8()}};
    if (!token.isEmpty()) result.append({"X-Plex-Token", token.toUtf8()});
    return result;
}

void PlexApiClient::discoverServers(const QString &accountToken,
                                    std::function<void(PlexApiError, const QList<PlexServer> &)> completion) {
    QUrl url(ResourcesUrl);
    url.setQuery(QUrlQuery{{QStringLiteral("includeHttps"), QStringLiteral("1")},
                           {QStringLiteral("includeRelay"), QStringLiteral("1")},
                           {QStringLiteral("includeIPv6"), QStringLiteral("1")}});
    http_.send({.url = url, .headers = headers(accountToken)},
               [completion = std::move(completion)](const PlexHttpResponse &response) {
                   if (const auto error = ErrorForStatus(response.status); error != PlexApiError::None) {
                       completion(error, {});
                       return;
                   }
                   const auto document = QJsonDocument::fromJson(response.body);
                   if (!document.isArray()) {
                       completion(PlexApiError::InvalidResponse, {});
                       return;
                   }
                   QList<PlexServer> servers;
                   for (const auto &value : document.array()) {
                       const auto resource = value.toObject();
                       const auto provides = resource.value(QStringLiteral("provides")).toString().split(QLatin1Char(','));
                       if (!provides.contains(QStringLiteral("server"))) continue;
                       PlexServer server{.name = resource.value(QStringLiteral("name")).toString(),
                                         .clientIdentifier = resource.value(QStringLiteral("clientIdentifier")).toString(),
                                         .accessToken = resource.value(QStringLiteral("accessToken")).toString(),
                                         .owned = resource.value(QStringLiteral("owned")).toBool()};
                       for (const auto &connectionValue : resource.value(QStringLiteral("connections")).toArray()) {
                           const auto connection = connectionValue.toObject();
                           server.connections.append({.uri = QUrl(connection.value(QStringLiteral("uri")).toString()),
                                                      .local = connection.value(QStringLiteral("local")).toBool(),
                                                      .relay = connection.value(QStringLiteral("relay")).toBool()});
                       }
                       if (!server.clientIdentifier.isEmpty()) servers.append(std::move(server));
                   }
                   completion(PlexApiError::None, servers);
               });
}

void PlexApiClient::listUsers(const PlexServer &server,
                              std::function<void(PlexApiError, const QList<PlexAccount> &)> completion) {
    if (!server.preferredUri().has_value()) {
        completion(PlexApiError::Unavailable, {});
        return;
    }
    http_.send({.url = ServerUrl(server, QStringLiteral("/accounts")), .headers = headers(server.accessToken)},
               [completion = std::move(completion)](const PlexHttpResponse &response) {
                   if (const auto error = ErrorForStatus(response.status); error != PlexApiError::None) {
                       completion(error, {});
                       return;
                   }
                   const auto container = MediaContainer(response.body);
                   if (!container.has_value()) {
                       completion(PlexApiError::InvalidResponse, {});
                       return;
                   }
                   QList<PlexAccount> accounts;
                   for (const auto &value : container->value(QStringLiteral("Account")).toArray()) {
                       const auto account = value.toObject();
                       const auto name = account.value(QStringLiteral("name")).toString();
                       // Account 0 is the server's unnamed system account.
                       if (name.isEmpty()) continue;
                       accounts.append({.id = Text(account.value(QStringLiteral("id"))), .name = name});
                   }
                   completion(PlexApiError::None, accounts);
               });
}

void PlexApiClient::listSessions(const PlexServer &server,
                                 std::function<void(PlexApiError, const QList<PlexSession> &)> completion) {
    if (!server.preferredUri().has_value()) {
        completion(PlexApiError::Unavailable, {});
        return;
    }
    http_.send({.url = ServerUrl(server, QStringLiteral("/status/sessions")), .headers = headers(server.accessToken)},
               [completion = std::move(completion)](const PlexHttpResponse &response) {
                   if (const auto error = ErrorForStatus(response.status); error != PlexApiError::None) {
                       completion(error, {});
                       return;
                   }
                   const auto container = MediaContainer(response.body);
                   if (!container.has_value()) {
                       completion(PlexApiError::InvalidResponse, {});
                       return;
                   }
                   QList<PlexSession> sessions;
                   for (const auto &value : container->value(QStringLiteral("Metadata")).toArray()) {
                       const auto item = value.toObject();
                       const auto sessionKey = Text(item.value(QStringLiteral("sessionKey")));
                       if (sessionKey.isEmpty()) continue;
                       const auto user = item.value(QStringLiteral("User")).toObject();
                       const auto player = item.value(QStringLiteral("Player")).toObject();
                       sessions.append({.sessionKey = sessionKey,
                                        .type = item.value(QStringLiteral("type")).toString(),
                                        .showTitle = item.value(QStringLiteral("grandparentTitle")).toString(),
                                        .episodeTitle = item.value(QStringLiteral("title")).toString(),
                                        .season = OptionalIndex(item.value(QStringLiteral("parentIndex"))),
                                        .episode = OptionalIndex(item.value(QStringLiteral("index"))),
                                        .viewOffsetMs = Milliseconds(item.value(QStringLiteral("viewOffset"))),
                                        .durationMs = Milliseconds(item.value(QStringLiteral("duration"))),
                                        .userId = Text(user.value(QStringLiteral("id"))),
                                        .userName = user.value(QStringLiteral("title")).toString(),
                                        .playerId = player.value(QStringLiteral("machineIdentifier")).toString(),
                                        .playerTitle = player.value(QStringLiteral("title")).toString(),
                                        .state = player.value(QStringLiteral("state")).toString()});
                   }
                   completion(PlexApiError::None, sessions);
               });
}
