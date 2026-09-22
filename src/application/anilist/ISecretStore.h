#ifndef HAIKENANIME_ISECRETSTORE_H
#define HAIKENANIME_ISECRETSTORE_H

#include <QString>

#include "../../domain/anilist/AniListCredentials.h"

class ISecretStore {
public:
    virtual ~ISecretStore() = default;

    [[nodiscard]] virtual bool loadAniListCredentials(AniListCredentials &credentials,
                                                       QString &error) = 0;
    virtual bool saveAniListCredentials(const AniListCredentials &credentials,
                                        QString &error) = 0;
};

#endif // HAIKENANIME_ISECRETSTORE_H
