#include "AlternativeNames.h"

#include <QSet>

namespace {
QString NameKey(const QString &name) {
    return name.trimmed().toCaseFolded();
}
}

QStringList MergeAlternativeNames(const QStringList &existing, const QStringList &incoming) {
    QStringList merged = existing;
    QSet<QString> seen;
    for (const auto &name : existing) {
        seen.insert(NameKey(name));
    }
    for (const auto &name : incoming) {
        const auto key = NameKey(name);
        if (key.isEmpty() || seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        merged.append(name);
    }
    return merged;
}
