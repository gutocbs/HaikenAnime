#ifndef HAIKENANIME_QTLOCALFILEOPENER_H
#define HAIKENANIME_QTLOCALFILEOPENER_H

#include "../../application/library/ILocalFileOpener.h"
#include <functional>

class QUrl;

class QtLocalFileOpener final : public ILocalFileOpener {
public:
    using OpenUrl = std::function<bool(const QUrl &)>;
    explicit QtLocalFileOpener(OpenUrl openUrl = {});
    bool open(const QString &path, QString &error) override;
private:
    OpenUrl openUrl_;
};

#endif
