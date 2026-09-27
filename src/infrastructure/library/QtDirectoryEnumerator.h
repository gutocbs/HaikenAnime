#ifndef HAIKENANIME_QTDIRECTORYENUMERATOR_H
#define HAIKENANIME_QTDIRECTORYENUMERATOR_H

#include "IDirectoryEnumerator.h"
#include <memory>

// Native listing result: a name with zero error, or a Windows terminal/error code.
// ERROR_NO_MORE_FILES is normal EOF; other errors must make traversal incomplete.
struct DirectoryListingEntry final {
    QString fileName;
    quint32 errorCode = 0;
};

class IDirectoryListing {
public:
    virtual ~IDirectoryListing() = default;
    virtual DirectoryListingEntry next() = 0;
};

class QtDirectoryEnumerator final : public IDirectoryEnumerator {
public:
    using ListingFactory = std::function<std::unique_ptr<IDirectoryListing>(const QString &path)>;
    explicit QtDirectoryEnumerator(ListingFactory listingFactory = {});
    bool enumerate(const QString &path,
        const std::function<bool(const QFileInfo &)> &visitor, QString &error) override;
private:
    ListingFactory listingFactory_;
};

#endif
