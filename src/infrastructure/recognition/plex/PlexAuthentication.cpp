#include "PlexAuthentication.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

#include <utility>

namespace {
const auto PinsUrl = QStringLiteral("https://plex.tv/api/v2/pins");

QString Translate(const char *text) {
    return QCoreApplication::translate("PlexAuthentication", text);
}
}

PlexAuthentication::PlexAuthentication(PlexAuthenticationDependencies dependencies)
    : dependencies_(std::move(dependencies)) {}

QUrl PlexAuthentication::authUrl(const QString &clientIdentifier, const QString &code, const QString &product) {
    const auto encode = [](const QString &value) { return QString::fromLatin1(QUrl::toPercentEncoding(value)); };
    // Parameters go in the fragment, exactly as documented for app.plex.tv/auth.
    QUrl url(QStringLiteral("https://app.plex.tv/auth"));
    url.setFragment(QStringLiteral("?clientID=%1&code=%2&context%5Bdevice%5D%5Bproduct%5D=%3")
                        .arg(encode(clientIdentifier), encode(code), encode(product)),
                    QUrl::TolerantMode);
    return url;
}

void PlexAuthentication::beginBrowserLogin(std::function<void(const PlexLoginResult &)> completion) {
    if (isLoginInProgress()) finish(PlexLoginStatus::Cancelled, Translate("A newer Plex login replaced this one."));
    completion_ = std::move(completion);
    const auto generation = ++generation_;
    if (dependencies_.http == nullptr || dependencies_.secrets == nullptr) {
        finish(PlexLoginStatus::Failed, Translate("Plex integration is not configured."));
        return;
    }
    PlexApiClient api(*dependencies_.http, dependencies_.identity);
    QUrl url(PinsUrl);
    url.setQuery(QUrlQuery{{QStringLiteral("strong"), QStringLiteral("true")}});
    dependencies_.http->send({.method = "POST", .url = url, .headers = api.headers({})},
                             [this, generation](const PlexHttpResponse &response) {
        if (generation != generation_) return;
        const auto pin = QJsonDocument::fromJson(response.body).object();
        const auto id = pin.value(QStringLiteral("id")).toInteger();
        const auto code = pin.value(QStringLiteral("code")).toString();
        if (response.status < 200 || response.status >= 300 || id == 0 || code.isEmpty()) {
            finish(PlexLoginStatus::Failed, Translate("Plex login could not be started."));
            return;
        }
        pinId_ = id;
        pinCode_ = code;
        const auto lifetime = pin.value(QStringLiteral("expiresIn")).toInt(DefaultPinLifetimeSeconds);
        expiresAt_ = now().addSecs(lifetime > 0 ? lifetime : DefaultPinLifetimeSeconds);
        const auto url = authUrl(dependencies_.identity.clientIdentifier, code, dependencies_.identity.product);
        if (!dependencies_.openUrl || !dependencies_.openUrl(url)) {
            finish(PlexLoginStatus::Failed, Translate("The browser could not be opened for the Plex login."));
            return;
        }
        if (dependencies_.schedule) {
            dependencies_.schedule(PollIntervalMs, [this, generation] {
                if (generation == generation_ && isLoginInProgress()) pollLogin();
            });
        }
    });
}

void PlexAuthentication::pollLogin() {
    if (!isLoginInProgress()) return;
    const auto generation = generation_;
    PlexApiClient api(*dependencies_.http, dependencies_.identity);
    QUrl url(PinsUrl + QStringLiteral("/%1").arg(pinId_));
    url.setQuery(QUrlQuery{{QStringLiteral("code"), pinCode_}});
    dependencies_.http->send({.url = url, .headers = api.headers({})}, [this, generation](const PlexHttpResponse &response) {
        if (generation != generation_ || !isLoginInProgress()) return;
        if (response.status == 404 || response.status == 410) {
            finish(PlexLoginStatus::TimedOut, Translate("The Plex login expired. Try again."));
            return;
        }
        const auto token = QJsonDocument::fromJson(response.body).object().value(QStringLiteral("authToken")).toString();
        if (response.status >= 200 && response.status < 300 && !token.isEmpty()) {
            QString storeError;
            if (!dependencies_.secrets->writeSecret(PluginId, SecretKey, token, storeError)) {
                finish(PlexLoginStatus::Failed, Translate("The Plex login could not be stored securely."));
                return;
            }
            finish(PlexLoginStatus::Succeeded, Translate("Connected to Plex."));
            return;
        }
        // Not claimed yet, or a transient failure: keep polling until the PIN expires.
        if (now() >= expiresAt_) {
            finish(PlexLoginStatus::TimedOut, Translate("The Plex login expired. Try again."));
            return;
        }
        if (dependencies_.schedule) {
            dependencies_.schedule(PollIntervalMs, [this, generation] {
                if (generation == generation_ && isLoginInProgress()) pollLogin();
            });
        }
    });
}

void PlexAuthentication::cancel() {
    if (isLoginInProgress()) finish(PlexLoginStatus::Cancelled, Translate("Plex login cancelled."));
}

bool PlexAuthentication::disconnect(QString &error) {
    error.clear();
    cancel();
    QString storeError;
    if (dependencies_.secrets == nullptr || !dependencies_.secrets->clearSecret(PluginId, SecretKey, storeError)) {
        error = Translate("The Plex login could not be removed.");
        return false;
    }
    return true;
}

bool PlexAuthentication::hasToken() const {
    return dependencies_.secrets != nullptr && dependencies_.secrets->hasSecret(PluginId, SecretKey);
}

bool PlexAuthentication::readToken(QString &token, QString &error) const {
    error.clear();
    QString storeError;
    if (dependencies_.secrets == nullptr || !dependencies_.secrets->readSecret(PluginId, SecretKey, token, storeError)) {
        token.clear();
        error = Translate("Not connected to Plex.");
        return false;
    }
    return true;
}

void PlexAuthentication::finish(const PlexLoginStatus status, const QString &message) {
    ++generation_;
    pinId_ = 0;
    pinCode_.clear();
    if (auto completion = std::exchange(completion_, {})) completion({.status = status, .safeMessage = message});
}

QDateTime PlexAuthentication::now() const {
    return dependencies_.clock ? dependencies_.clock() : QDateTime::currentDateTimeUtc();
}
