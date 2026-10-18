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

    // Folder key used to compare media folders with file paths: forward slashes, cleaned, no trailing
    // slash, and case-folded on Windows where paths are case-insensitive.
    static QString NormalizeFolder(const QString &path);

    // Ids of the media whose LocalPath is the deepest folder containing the file (empty when none).
    // Folder boundaries are respected, so "Foo" never contains "Foo 2/episode.mkv".
    QList<int> findByFolder(const QString &absoluteFilePath) const;

    // Ids of the media entries (one per catalog entry, catalog order) matching the normalized title.
    QList<int> find(const QString &normalizedTitle) const;
    // Ids among find() results that can be an anime: manga and novels share titles with their adaptations.
    QList<int> findAnime(const QString &normalizedTitle) const;
    qsizetype titleCount() const { return titles_.size(); }

private:
    QHash<int, MediaType> types_;
    QHash<QString, QList<int>> titles_;
    QHash<QString, QList<int>> folders_;
};

#endif // HAIKENANIME_LOCALMEDIACATALOGINDEX_H
