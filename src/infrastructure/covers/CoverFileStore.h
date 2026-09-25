#ifndef HAIKENANIME_COVERFILESTORE_H
#define HAIKENANIME_COVERFILESTORE_H

#include "../../application/covers/CoverSettings.h"
#include "../../application/covers/ICoverFileStore.h"

class CoverFileStore final : public ICoverFileStore {
public:
    CoverFileStore(QString rootPath, CoverSettings settings);

    bool Exists(const QString &relativePath) const override;
    bool Publish(int mediaId, const QString &remoteUrl,
                 const QString &temporaryPath, const QString &responseMime,
                 PublishedCover &result, QString &error) override;
    bool Remove(const QString &relativePath, QString &error) override;
    bool Clear(QString &error) override;
    bool RemoveOrphans(const QSet<QString> &referencedPaths,
                       int maxFiles, int &removedFiles, QString &error) override;
    QString AbsolutePath(const QString &relativePath) const override;

private:
    bool IsSafeRelativePath(const QString &relativePath) const;

    QString rootPath_;
    CoverSettings settings_;
};

#endif // HAIKENANIME_COVERFILESTORE_H
