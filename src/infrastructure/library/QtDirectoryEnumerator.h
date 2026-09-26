#ifndef HAIKENANIME_QTDIRECTORYENUMERATOR_H
#define HAIKENANIME_QTDIRECTORYENUMERATOR_H

#include "IDirectoryEnumerator.h"

class QtDirectoryEnumerator final : public IDirectoryEnumerator {
public:
    bool enumerate(const QString &path,
        const std::function<bool(const QFileInfo &)> &visitor, QString &error) override;
};

#endif
