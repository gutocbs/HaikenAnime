#ifndef HAIKENANIME_FILESECRETSTORE_H
#define HAIKENANIME_FILESECRETSTORE_H

#include <QString>

#include "../../application/anilist/ISecretStore.h"

/** Temporary plain-text store; production use requires a secure replacement. */
class FileSecretStore final : public ISecretStore {
public:
    /** Creates a store using the supplied path or the application config path by default. */
    explicit FileSecretStore(QString filePath = {});

    /** Loads credentials from the configured file without decrypting them. */
    [[nodiscard]] bool loadAniListCredentials(AniListCredentials &credentials,
                                               QString &error) override;

    /** Writes credentials atomically to the configured plain-text file. */
    bool saveAniListCredentials(const AniListCredentials &credentials,
                                QString &error) override;

    /** Returns the path used by this store. */
    [[nodiscard]] QString filePath() const;

private:
    QString filePath_;
};

#endif // HAIKENANIME_FILESECRETSTORE_H
