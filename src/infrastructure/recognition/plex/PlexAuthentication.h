#ifndef HAIKENANIME_PLEXAUTHENTICATION_H
#define HAIKENANIME_PLEXAUTHENTICATION_H

#include <QDateTime>
#include <QString>
#include <QUrl>

#include <functional>

#include "../../../application/recognition/IPluginSecretStore.h"
#include "PlexApiClient.h"

enum class PlexLoginStatus { Succeeded, TimedOut, Failed, Cancelled };

struct PlexLoginResult final {
    PlexLoginStatus status = PlexLoginStatus::Failed;
    /// User-facing; never contains the PIN token.
    QString safeMessage;
};

struct PlexAuthenticationDependencies final {
    IPlexHttp *http = nullptr;
    IPluginSecretStore *secrets = nullptr;
    PlexClientIdentity identity;
    std::function<bool(const QUrl &)> openUrl;
    std::function<void(int delayMs, std::function<void()>)> schedule;
    std::function<QDateTime()> clock;
};

/// Browser PIN login (legacy token flow): create a PIN, open app.plex.tv/auth, poll until the
/// PIN carries authToken, then store the token in the secret store. The token never leaves here.
class PlexAuthentication final {
public:
    static constexpr int PollIntervalMs = 1000;
    static constexpr int DefaultPinLifetimeSeconds = 1800;
    static inline const QString SecretKey = QStringLiteral("token");
    static inline const QString PluginId = QStringLiteral("plex");

    explicit PlexAuthentication(PlexAuthenticationDependencies dependencies);

    void beginBrowserLogin(std::function<void(const PlexLoginResult &)> completion);
    /// One poll of the pending PIN; called on a timer while a login is in progress.
    void pollLogin();
    void cancel();
    bool disconnect(QString &error);
    [[nodiscard]] bool hasToken() const;
    [[nodiscard]] bool isLoginInProgress() const { return pinId_ != 0; }
    /// For infrastructure adapters (server discovery) only.
    bool readToken(QString &token, QString &error) const;

    [[nodiscard]] static QUrl authUrl(const QString &clientIdentifier, const QString &code, const QString &product);

private:
    void finish(PlexLoginStatus status, const QString &message);
    [[nodiscard]] QDateTime now() const;

    PlexAuthenticationDependencies dependencies_;
    std::function<void(const PlexLoginResult &)> completion_;
    qint64 pinId_ = 0;
    QString pinCode_;
    QDateTime expiresAt_;
    quint64 generation_ = 0;
};

#endif // HAIKENANIME_PLEXAUTHENTICATION_H
