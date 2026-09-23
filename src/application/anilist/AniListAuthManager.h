#ifndef HAIKENANIME_ANILISTAUTHMANAGER_H
#define HAIKENANIME_ANILISTAUTHMANAGER_H

#include "IAniListAuthProvider.h"
#include "ISecretStore.h"

/** Loads and caches AniList credentials while keeping storage separate from API transport. */
class AniListAuthManager final : public IAniListAuthProvider {
public:
    /** Creates an authorization manager backed by the supplied secret store. */
    explicit AniListAuthManager(ISecretStore &secretStore);

    /** Loads credentials from storage and writes any failure description to error. */
    [[nodiscard]] bool load(QString &error);

    /** Saves credentials through the configured store and updates the in-memory cache. */
    [[nodiscard]] bool save(const AniListCredentials &credentials, QString &error);

    /** Returns the last successfully loaded or saved credentials. */
    [[nodiscard]] AniListCredentials credentials() const override;

private:
    ISecretStore &secretStore_;
    AniListCredentials credentials_;
};

#endif // HAIKENANIME_ANILISTAUTHMANAGER_H
