#ifndef HAIKENANIME_ILOCALFILEOPENER_H
#define HAIKENANIME_ILOCALFILEOPENER_H

#include <QString>

class ILocalFileOpener {
public:
    virtual ~ILocalFileOpener() = default;
    virtual bool open(const QString &path, QString &error) = 0;
};

#endif
