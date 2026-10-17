#ifndef HAIKENANIME_ALTERNATIVENAMES_H
#define HAIKENANIME_ALTERNATIVENAMES_H

#include <QStringList>

/// Merges source-provided alternative names into the stored ones.
/// Existing names keep their order; incoming names not already present
/// (trimmed, case-insensitive comparison) are appended. Blank names are ignored.
[[nodiscard]] QStringList MergeAlternativeNames(const QStringList &existing, const QStringList &incoming);

#endif
