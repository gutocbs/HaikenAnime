#ifndef HAIKENANIME_ISECRETSTORE_H
#define HAIKENANIME_ISECRETSTORE_H

#include <QString>

#include "AniListCredentials.h"

class ISecretStore {
public:
    virtual ~ISecretStore() = default;

    /**
     * Stable storage contract for credentials. Implementations may later encrypt or use SQLite
     * without changing authorization or transport consumers.
     */
    /** Replaces credentials with the stored value and clears error on success. */
    [[nodiscard]] virtual bool loadAniListCredentials(AniListCredentials &credentials,
                                                       QString &error) = 0;

    /** Stores the AniList credentials and clears error on success. */
    virtual bool saveAniListCredentials(const AniListCredentials &credentials,
                                        QString &error) = 0;
};

#endif // HAIKENANIME_ISECRETSTORE_H
