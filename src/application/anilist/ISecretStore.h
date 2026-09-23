#ifndef HAIKENANIME_ISECRETSTORE_H
#define HAIKENANIME_ISECRETSTORE_H

#include <QString>

#include "../../domain/anilist/AniListCredentials.h"

class ISecretStore {
public:
    virtual ~ISecretStore() = default;

    /**
     * Stable storage contract for credentials. Implementations may later encrypt or use SQLite
     * without changing authorization or transport consumers.
     */
    /** Loads the stored AniList credentials and reports failures through error. */
    [[nodiscard]] virtual bool loadAniListCredentials(AniListCredentials &credentials,
                                                       QString &error) = 0;

    /** Stores the AniList credentials and reports failures through error. */
    virtual bool saveAniListCredentials(const AniListCredentials &credentials,
                                        QString &error) = 0;
};

#endif // HAIKENANIME_ISECRETSTORE_H
