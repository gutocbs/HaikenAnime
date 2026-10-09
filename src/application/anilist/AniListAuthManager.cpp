#include "AniListAuthManager.h"

AniListAuthManager::AniListAuthManager(ISecretStore &secretStore)
    : secretStore_(secretStore) {
}

bool AniListAuthManager::load(QString &error) {
    error.clear();
    AniListCredentials loadedCredentials;
    if (!secretStore_.loadAniListCredentials(loadedCredentials, error)) {
        return false;
    }

    credentials_ = loadedCredentials;
    state_ = AniListAuthenticationState::Authenticated;
    return true;
}

bool AniListAuthManager::save(const AniListCredentials &credentials, QString &error) {
    error.clear();
    if (!secretStore_.saveAniListCredentials(credentials, error)) {
        return false;
    }

    credentials_ = credentials;
    state_ = AniListAuthenticationState::Authenticated;
    return true;
}

bool AniListAuthManager::clear(QString &error) {
    error.clear();
    if (!secretStore_.clearAniListCredentials(error)) {
        return false;
    }

    credentials_ = {};
    state_ = AniListAuthenticationState::Disconnected;
    return true;
}

QUrl AniListAuthManager::beginAuthorization(const AniListOAuthConfig &config) {
    state_ = AniListAuthenticationState::Authorizing;
    return config.authorizationUrl();
}

bool AniListAuthManager::handleCallback(const QUrl &callback, QString &error) {
    AniListOAuthCallbackParser parser;
    AniListCredentials callbackCredentials;
    if (!parser.parse(callback, callbackCredentials, error)) {
        state_ = AniListAuthenticationState::AuthenticationFailed;
        return false;
    }

    credentials_ = callbackCredentials;
    state_ = AniListAuthenticationState::AwaitingValidation;
    return true;
}

bool AniListAuthManager::validateToken(IAniListViewerClient &viewerClient, QString &error) {
    AniListViewer viewer;
    if (!viewerClient.loadViewer(viewer, error) || viewer.id <= 0 || viewer.username.trimmed().isEmpty()) {
        if (error.isEmpty()) {
            error = QStringLiteral("AniList Viewer validation returned invalid account data.");
        }
        state_ = AniListAuthenticationState::AuthenticationFailed;
        return false;
    }
    credentials_.userId = viewer.id;
    credentials_.username = viewer.username;
    if (!save(credentials_, error)) {
        state_ = AniListAuthenticationState::AuthenticationFailed;
        return false;
    }
    return true;
}

AniListAuthenticationState AniListAuthManager::state() const {
    return state_;
}

AniListCredentials AniListAuthManager::credentials() const {
    return credentials_;
}
