#include "LocalMediaCatalogIndex.h"

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
    }
}

QList<int> LocalMediaCatalogIndex::find(const QString &normalizedTitle) const {
    return titles_.value(normalizedTitle);
}
