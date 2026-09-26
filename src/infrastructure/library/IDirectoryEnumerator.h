#ifndef HAIKENANIME_IDIRECTORYENUMERATOR_H
#define HAIKENANIME_IDIRECTORYENUMERATOR_H

#include <QFileInfo>
#include <functional>

class IDirectoryEnumerator {
public:
    virtual ~IDirectoryEnumerator() = default;
    // Streams immediate children, excluding dot entries, without recursion or content
    // reads. A false visitor result stops enumeration successfully. Directory access
    // failure returns false and fills error; already visited entries remain valid.
    virtual bool enumerate(const QString &path,
        const std::function<bool(const QFileInfo &)> &visitor, QString &error) = 0;
};

#endif
