#ifndef HAIKENANIME_COVERTEMPORARYSTORE_H
#define HAIKENANIME_COVERTEMPORARYSTORE_H

#include <QString>

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

#endif // HAIKENANIME_COVERTEMPORARYSTORE_H
