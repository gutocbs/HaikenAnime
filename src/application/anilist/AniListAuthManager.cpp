#include "AniListAuthManager.h"

AniListAuthManager::AniListAuthManager(ISecretStore &secretStore)
    : secretStore_(secretStore) {
}

void AniListAuthManager::setAuditLogger(AuditLogger logger) {
    auditLogger_ = std::move(logger);
}

bool AniListAuthManager::load(QString &error) {
    error.clear();
    AniListCredentials loadedCredentials;
    if (!secretStore_.loadAniListCredentials(loadedCredentials, error)) {
        audit(QStringLiteral("AniList credentials could not be restored."));
        return false;
    }

    credentials_ = loadedCredentials;
    state_ = AniListAuthenticationState::Authenticated;
    audit(QStringLiteral("AniList credentials restored."));
    return true;
}

bool AniListAuthManager::save(const AniListCredentials &credentials, QString &error) {
    error.clear();
    if (!secretStore_.saveAniListCredentials(credentials, error)) {
        audit(QStringLiteral("AniList credentials could not be saved."));
        return false;
    }

    credentials_ = credentials;
    state_ = AniListAuthenticationState::Authenticated;
    audit(QStringLiteral("AniList credentials saved."));
    return true;
}

bool AniListAuthManager::clear(QString &error) {
    error.clear();
    if (!secretStore_.clearAniListCredentials(error)) {
        audit(QStringLiteral("AniList credentials could not be cleared."));
        return false;
    }

    credentials_ = {};
    state_ = AniListAuthenticationState::Disconnected;
    audit(QStringLiteral("AniList credentials cleared."));
    return true;
}

QUrl AniListAuthManager::beginAuthorization(const AniListOAuthConfig &config) {
    state_ = AniListAuthenticationState::Authorizing;
    audit(QStringLiteral("AniList OAuth authorization started."));
    return config.authorizationUrl();
}

bool AniListAuthManager::handleCallback(const QUrl &callback, QString &error) {
    AniListOAuthCallbackParser parser;
    AniListCredentials callbackCredentials;
    if (!parser.parse(callback, callbackCredentials, error)) {
        state_ = AniListAuthenticationState::AuthenticationFailed;
        audit(QStringLiteral("AniList OAuth callback was rejected."));
        return false;
    }

    credentials_ = callbackCredentials;
    state_ = AniListAuthenticationState::AwaitingValidation;
    audit(QStringLiteral("AniList OAuth callback received."));
    audit(QStringLiteral("TEMPORARY DIAGNOSTIC AniList OAuth access token: %1").arg(credentials_.token));
    return true;
}

bool AniListAuthManager::validateToken(IAniListViewerClient &viewerClient, QString &error) {
    audit(QStringLiteral("AniList Viewer validation started."));
    AniListViewer viewer;
    if (!viewerClient.loadViewer(viewer, error) || viewer.id <= 0 || viewer.username.trimmed().isEmpty()) {
        if (error.isEmpty()) {
            error = QStringLiteral("AniList Viewer validation returned invalid account data.");
        }
        state_ = AniListAuthenticationState::AuthenticationFailed;
        audit(QStringLiteral("AniList Viewer validation failed."));
        return false;
    }
    credentials_.userId = viewer.id;
    credentials_.username = viewer.username;
    if (!save(credentials_, error)) {
        state_ = AniListAuthenticationState::AuthenticationFailed;
        audit(QStringLiteral("AniList Viewer validation could not persist credentials."));
        return false;
    }
    audit(QStringLiteral("AniList Viewer validation succeeded."));
    return true;
}

AniListAuthenticationState AniListAuthManager::state() const {
    return state_;
}

AniListCredentials AniListAuthManager::credentials() const {
    return credentials_;
}

void AniListAuthManager::audit(const QString &event) const {
    if (auditLogger_) auditLogger_(event);
}
