#ifndef HAIKENANIME_ICOVERFILESTORE_H
#define HAIKENANIME_ICOVERFILESTORE_H

#include <QSet>
#include <QString>

struct PublishedCover {
    QString relativePath;
    QString mimeType;
    qint64 byteSize = 0;
};

class ICoverFileStore {
public:
    virtual ~ICoverFileStore() = default;
    virtual bool Exists(const QString &relativePath) const = 0;
    virtual bool Publish(int mediaId, const QString &remoteUrl,
                         const QString &temporaryPath, const QString &responseMime,
                         PublishedCover &result, QString &error) = 0;
    virtual bool Remove(const QString &relativePath, QString &error) = 0;
    virtual bool Clear(QString &error) = 0;
    virtual bool RemoveOrphans(const QSet<QString> &referencedPaths,
                               int maxFiles, int &removedFiles, QString &error) = 0;
    virtual QString AbsolutePath(const QString &relativePath) const = 0;
};

#endif // HAIKENANIME_ICOVERFILESTORE_H
