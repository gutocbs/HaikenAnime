#ifndef HAIKENANIME_LOCALMEDIACATALOGINDEX_H
#define HAIKENANIME_LOCALMEDIACATALOGINDEX_H

#include "../../domain/media/Media.h"

#include <QHash>
#include <QList>
#include <QString>

// Maps a normalized title to the ids of the catalog media that carry it (Name, EnglishName,
// OriginalName or any AlternativeName). Built once per recognition run so each local file costs one
// normalization and one lookup instead of re-normalizing the whole catalog.
class LocalMediaCatalogIndex final {
public:
    LocalMediaCatalogIndex() = default;
    explicit LocalMediaCatalogIndex(const QList<Media> &catalog);

    // Case-folded title without apostrophes, with punctuation and whitespace collapsed to one space.
    static QString NormalizeTitle(const QString &value);

    // Ids of the media entries (one per catalog entry, catalog order) matching the normalized title.
    QList<int> find(const QString &normalizedTitle) const;
    qsizetype titleCount() const { return titles_.size(); }

private:
    QHash<QString, QList<int>> titles_;
};

#endif // HAIKENANIME_LOCALMEDIACATALOGINDEX_H
