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
    state_ = AniListAuthenticationState::AwaitingValidation;
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

AniListAuthenticationState AniListAuthManager::state() const {
    return state_;
}

AniListCredentials AniListAuthManager::credentials() const {
    return credentials_;
}
