#include "AniListAuthManager.h"

#include <QMutexLocker>

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
        // The store reports why (e.g. no account saved yet); never the credential itself.
        audit(QStringLiteral("AniList credentials could not be restored: %1").arg(error));
        return false;
    }

    SetSession(loadedCredentials, AniListAuthenticationState::Authenticated);
    audit(QStringLiteral("AniList credentials restored."));
    return true;
}

bool AniListAuthManager::save(const AniListCredentials &credentials, QString &error) {
    error.clear();
    if (!secretStore_.saveAniListCredentials(credentials, error)) {
        audit(QStringLiteral("AniList credentials could not be saved."));
        return false;
    }

    SetSession(credentials, AniListAuthenticationState::Authenticated);
    audit(QStringLiteral("AniList credentials saved."));
    return true;
}

bool AniListAuthManager::clear(QString &error) {
    error.clear();
    if (!secretStore_.clearAniListCredentials(error)) {
        audit(QStringLiteral("AniList credentials could not be cleared."));
        return false;
    }

    SetSession({}, AniListAuthenticationState::Disconnected);
    audit(QStringLiteral("AniList credentials cleared."));
    return true;
}

QUrl AniListAuthManager::beginAuthorization(const AniListOAuthConfig &config) {
    SetState(AniListAuthenticationState::Authorizing);
    audit(QStringLiteral("AniList OAuth authorization started."));
    return config.authorizationUrl();
}

bool AniListAuthManager::handleCallback(const QUrl &callback, QString &error) {
    AniListOAuthCallbackParser parser;
    AniListCredentials callbackCredentials;
    if (!parser.parse(callback, callbackCredentials, error)) {
        SetState(AniListAuthenticationState::AuthenticationFailed);
        audit(QStringLiteral("AniList OAuth callback was rejected."));
        return false;
    }

    SetSession(callbackCredentials, AniListAuthenticationState::AwaitingValidation);
    audit(QStringLiteral("AniList OAuth callback received."));
    return true;
}

bool AniListAuthManager::validateToken(IAniListViewerClient &viewerClient, QString &error) {
    audit(QStringLiteral("AniList Viewer validation started."));
    AniListViewer viewer;
    if (!viewerClient.loadViewer(viewer, error) || viewer.id <= 0 || viewer.username.trimmed().isEmpty()) {
        if (error.isEmpty()) {
            error = QStringLiteral("AniList Viewer validation returned invalid account data.");
        }
        SetState(AniListAuthenticationState::AuthenticationFailed);
        audit(QStringLiteral("AniList Viewer validation failed."));
        return false;
    }
    auto validated = StoredCredentials();
    validated.userId = viewer.id;
    validated.username = viewer.username;
    if (!save(validated, error)) {
        SetState(AniListAuthenticationState::AuthenticationFailed);
        audit(QStringLiteral("AniList Viewer validation could not persist credentials."));
        return false;
    }
    audit(QStringLiteral("AniList Viewer validation succeeded."));
    return true;
}

AniListAuthenticationState AniListAuthManager::state() const {
    QMutexLocker lock(&mutex_);
    return state_;
}

AniListCredentials AniListAuthManager::credentials() const {
    QMutexLocker lock(&mutex_);
    return credentialsSuspended_ ? AniListCredentials{} : credentials_;
}

void AniListAuthManager::suspendCredentials() {
    QMutexLocker lock(&mutex_);
    credentialsSuspended_ = true;
}

void AniListAuthManager::resumeCredentials() {
    QMutexLocker lock(&mutex_);
    credentialsSuspended_ = false;
}

AniListCredentials AniListAuthManager::StoredCredentials() const {
    QMutexLocker lock(&mutex_);
    return credentials_;
}

void AniListAuthManager::SetState(const AniListAuthenticationState state) {
    QMutexLocker lock(&mutex_);
    state_ = state;
}

void AniListAuthManager::SetSession(const AniListCredentials &credentials, const AniListAuthenticationState state) {
    QMutexLocker lock(&mutex_);
    credentials_ = credentials;
    state_ = state;
}

void AniListAuthManager::audit(const QString &event) const {
    if (auditLogger_) auditLogger_(event);
}
