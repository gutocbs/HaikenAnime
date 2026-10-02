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
    return true;
}

bool AniListAuthManager::save(const AniListCredentials &credentials, QString &error) {
    error.clear();
    if (!secretStore_.saveAniListCredentials(credentials, error)) {
        return false;
    }

    credentials_ = credentials;
    return true;
}

AniListCredentials AniListAuthManager::credentials() const {
    return credentials_;
}
