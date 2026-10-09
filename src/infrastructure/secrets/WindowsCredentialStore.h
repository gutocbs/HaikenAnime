#ifndef HAIKENANIME_WINDOWSCREDENTIALSTORE_H
#define HAIKENANIME_WINDOWSCREDENTIALSTORE_H

#include <QString>

#include "../../application/anilist/ISecretStore.h"

/** Stores one AniList session in Windows Credential Manager. */
class WindowsCredentialStore final : public ISecretStore {
public:
    explicit WindowsCredentialStore(QString targetName = QStringLiteral("HaikenAnime.AniList"));

    [[nodiscard]] bool loadAniListCredentials(AniListCredentials &credentials,
                                               QString &error) override;
    bool saveAniListCredentials(const AniListCredentials &credentials, QString &error) override;
    bool clearAniListCredentials(QString &error) override;

private:
    QString targetName_;
};

#endif // HAIKENANIME_WINDOWSCREDENTIALSTORE_H
