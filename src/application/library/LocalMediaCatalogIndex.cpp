#include "LocalMediaCatalogIndex.h"

#include <QDir>
#include <QRegularExpression>
#include <QSet>

QString LocalMediaCatalogIndex::NormalizeTitle(const QString &value) {
    static const QRegularExpression separatorExpression(
        QStringLiteral(R"([\s._:/\\-]+|[\p{P}\p{S}]+)")
    );
    static const QRegularExpression apostropheExpression(
        QStringLiteral("['\u2019\u02BC\uFF07]"));
    static const QRegularExpression whitespaceExpression(QStringLiteral(R"(\s+)"));

    auto normalized = value.trimmed().toCaseFolded();
    // Scene releases usually drop apostrophes, so remove them instead of splitting words.
    normalized.remove(apostropheExpression);
    normalized.replace(separatorExpression, QStringLiteral(" "));
    normalized.replace(whitespaceExpression, QStringLiteral(" "));
    return normalized.trimmed();
}

LocalMediaCatalogIndex::LocalMediaCatalogIndex(const QList<Media> &catalog) {
    for (const auto &media : catalog) {
        QSet<QString> mediaTitles;
        const auto add = [&mediaTitles](const QString &candidate) {
            if (candidate.trimmed().isEmpty()) return;
            const auto normalized = NormalizeTitle(candidate);
            if (!normalized.isEmpty()) mediaTitles.insert(normalized);
        };
        add(media.Name);
        add(media.EnglishName);
        add(media.OriginalName);
        for (const auto &alternative : media.AlternativeNames) add(alternative);
        // A media with several names mapping to one title is still a single candidate.
        for (const auto &title : mediaTitles) titles_[title].append(media.Id);
        types_.insert(media.Id, media.Type);
        const auto folder = NormalizeFolder(media.LocalPath);
        if (!folder.isEmpty()) {
            auto &ids = folders_[folder];
            if (!ids.contains(media.Id)) ids.append(media.Id);
        }
    }
}

QList<int> LocalMediaCatalogIndex::find(const QString &normalizedTitle) const {
    return titles_.value(normalizedTitle);
}

QList<int> LocalMediaCatalogIndex::findAnime(const QString &normalizedTitle) const {
    auto ids = titles_.value(normalizedTitle);
    ids.removeIf([this](const int id) {
        const auto type = types_.value(id, MediaType::Unknown);
        return type == MediaType::Manga || type == MediaType::Novel;
    });
    return ids;
}

QString LocalMediaCatalogIndex::NormalizeFolder(const QString &path) {
    auto normalized = path.trimmed();
    if (normalized.isEmpty()) return {};
    normalized.replace(QLatin1Char('\\'),QLatin1Char('/'));
    normalized = QDir::cleanPath(normalized);
    while (normalized.endsWith(QLatin1Char('/'))) normalized.chop(1);
    if (normalized.isEmpty() || normalized == QLatin1String(".")) return {};
#ifdef Q_OS_WIN
    normalized = normalized.toCaseFolded();
#endif
    return normalized;
}

QList<int> LocalMediaCatalogIndex::findByFolder(const QString &absoluteFilePath) const {
    if (folders_.isEmpty()) return {};
    const auto file = NormalizeFolder(absoluteFilePath);
    // Walk the parents from the deepest one, so the first hit is the deepest media folder.
    for (auto separator = file.lastIndexOf(QLatin1Char('/')); separator > 0;
         separator = file.lastIndexOf(QLatin1Char('/'), separator - 1)) {
        const auto found = folders_.constFind(file.left(separator));
        if (found != folders_.constEnd()) return found.value();
    }
    return {};
}
