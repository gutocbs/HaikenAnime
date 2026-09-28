#ifndef HAIKENANIME_COVERTEMPORARYSTORE_H
#define HAIKENANIME_COVERTEMPORARYSTORE_H

#include <QString>

#ifdef HAIKENANIME_COVER_TEMPORARY_STORE_TESTING
#include <functional>
#endif

namespace CoverTemporaryFiles {
inline constexpr auto NameTemplate = "cover-XXXXXX.tmp";
inline constexpr auto NameFilter = "cover-*.tmp";
}

class CoverTemporaryStore final {
public:
    explicit CoverTemporaryStore(QString rootPath);

    bool ClearAbandoned(int &removedFiles, QString &error) const;

private:
    QString rootPath_;
};

#ifdef HAIKENANIME_COVER_TEMPORARY_STORE_TESTING
namespace CoverTemporaryStoreTesting {
void SetRootValidatedCallback(std::function<void()> callback);
}
#endif

#endif // HAIKENANIME_COVERTEMPORARYSTORE_H
