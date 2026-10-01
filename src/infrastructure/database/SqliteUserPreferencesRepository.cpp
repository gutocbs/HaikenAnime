#include "SqliteUserPreferencesRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include "../../application/configuration/UserPreferencesValidator.h"

SqliteUserPreferencesRepository::SqliteUserPreferencesRepository(
    QSqlDatabase database, QString readQuery, QString upsertQuery)
    : database_(std::move(database)), readQuery_(std::move(readQuery)),
      upsertQuery_(std::move(upsertQuery)) {}

bool SqliteUserPreferencesRepository::read(UserPreferences &preferences, bool &found,
                                           QString &error) {
    error.clear();
    found = false;
    QSqlQuery query(database_);
    if (!query.exec(readQuery_)) {
        error = query.lastError().text();
        return false;
    }
    if (!query.next()) return true;
    const auto quality = ParseCoverQuality(query.value(3).toString());
    if (!quality.has_value()) {
        error = QStringLiteral("Stored cover quality is unsupported.");
        return false;
    }
    UserPreferences loaded;
    loaded.scoreMinimum = query.value(0).toDouble();
    loaded.scoreMaximum = query.value(1).toDouble();
    loaded.scoreStep = query.value(2).toDouble();
    loaded.coverQuality = quality.value();
    loaded.synchronizationEnabled = query.value(4).toBool();
    loaded.synchronizationIntervalMs = query.value(5).toInt();
    loaded.libraryRoot = query.value(6).toString();
    QJsonParseError parseError;
    const auto extensions = QJsonDocument::fromJson(query.value(7).toString().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !extensions.isArray()) {
        error = QStringLiteral("Stored scan extensions must be a JSON array of strings.");
        return false;
    }
    loaded.scanExtensions.clear();
    for (const auto &extension : extensions.array()) {
        if (!extension.isString()) {
            error = QStringLiteral("Stored scan extensions must be strings.");
            return false;
        }
        loaded.scanExtensions.append(extension.toString());
    }
    loaded.homeSortKey = query.value(8).toString();
    loaded.cardStatusPresentation = ParseCardStatusPresentation(query.value(9).toString())
                                        .value_or(CardStatusPresentation::PersonalListStatus);
    loaded.languageKey = NormalizeLanguageKey(query.value(10).toString());
    loaded.preferredTitleKey = NormalizePreferredTitleKey(query.value(11).toString());
    const auto validation = ValidateUserPreferences(loaded);
    if (!validation.valid) {
        error = validation.error;
        return false;
    }
    loaded.scanExtensions = NormalizeScanExtensions(loaded.scanExtensions, error);
    preferences = loaded;
    found = true;
    return true;
}

bool SqliteUserPreferencesRepository::replace(const UserPreferences &preferences, QString &error) {
    error.clear();
    const auto validation = ValidateUserPreferences(preferences);
    if (!validation.valid) {
        error = validation.error;
        return false;
    }
    if (!database_.transaction()) {
        error = database_.lastError().text();
        return false;
    }
    QSqlQuery query(database_);
    if (!query.prepare(upsertQuery_)) {
        error = query.lastError().text();
        database_.rollback();
        return false;
    }
    query.bindValue(QStringLiteral(":score_minimum"), preferences.scoreMinimum);
    query.bindValue(QStringLiteral(":score_maximum"), preferences.scoreMaximum);
    query.bindValue(QStringLiteral(":score_step"), preferences.scoreStep);
    query.bindValue(QStringLiteral(":cover_quality"), CoverQualityName(preferences.coverQuality));
    query.bindValue(QStringLiteral(":synchronization_enabled"), preferences.synchronizationEnabled);
    query.bindValue(QStringLiteral(":synchronization_interval_ms"), preferences.synchronizationIntervalMs);
    query.bindValue(QStringLiteral(":home_sort_key"), preferences.homeSortKey);
    query.bindValue(QStringLiteral(":card_status_presentation"),
                    CardStatusPresentationKey(preferences.cardStatusPresentation));
    query.bindValue(QStringLiteral(":language_key"), preferences.languageKey);
    query.bindValue(QStringLiteral(":preferred_title_key"), preferences.preferredTitleKey);
    query.bindValue(QStringLiteral(":library_root"), preferences.libraryRoot);
    query.bindValue(QStringLiteral(":scan_extensions"), QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(NormalizeScanExtensions(preferences.scanExtensions, error)))
            .toJson(QJsonDocument::Compact)));
    if (!query.exec() || !database_.commit()) {
        error = query.lastError().text();
        if (error.isEmpty()) error = database_.lastError().text();
        database_.rollback();
        return false;
    }
    return true;
}
