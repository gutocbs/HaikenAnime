#include "SqliteUserPreferencesRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include "../../application/configuration/UserPreferencesValidator.h"
#include "SqlQueryStore.h"
#include "SqliteQueries.h"

namespace {
/** Parses a stored JSON array of strings; each message names the malformed value. */
bool ParseStringArray(const QString &json, QStringList &values, const QString &notArrayError,
                      const QString &notStringError, QString &error) {
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        error = notArrayError;
        return false;
    }
    values.clear();
    for (const auto &value : document.array()) {
        if (!value.isString()) {
            error = notStringError;
            return false;
        }
        values.append(value.toString());
    }
    return true;
}
}

SqliteUserPreferencesRepository::SqliteUserPreferencesRepository(QSqlDatabase database)
    : database_(std::move(database)) {}

bool SqliteUserPreferencesRepository::read(UserPreferences &preferences, bool &found,
                                           QString &error) {
    QString warning;
    return read(preferences, found, error, warning);
}

bool SqliteUserPreferencesRepository::read(UserPreferences &preferences, bool &found,
                                           QString &error, QString &warning) {
    error.clear();
    warning.clear();
    found = false;
    QString readQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::ReadUserPreferences, readQuery, error)) return false;
    QSqlQuery query(database_);
    if (!query.exec(readQuery)) {
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
    // library_roots (JSON array) replaced library_root. An empty array with a library_root means the row
    // predates library_roots; with an empty library_root it means no folder was configured.
    const auto storedRoots = query.value(16).toString();
    const auto legacyRoot = query.value(6).toString();
    loaded.libraryRoots = legacyRoot.isEmpty() ? QStringList{} : QStringList{legacyRoot};
    if (!storedRoots.isEmpty()) {
        QStringList roots;
        if (!ParseStringArray(storedRoots, roots,
                              QStringLiteral("Stored library roots must be a JSON array of strings."),
                              QStringLiteral("Stored library roots must be strings."), error)) {
            return false;
        }
        if (!roots.isEmpty()) loaded.libraryRoots = roots;
    }
    if (!ParseStringArray(query.value(7).toString(), loaded.scanExtensions,
                          QStringLiteral("Stored scan extensions must be a JSON array of strings."),
                          QStringLiteral("Stored scan extensions must be strings."), error)) {
        return false;
    }
    loaded.homeSortKey = query.value(8).toString();
    loaded.cardStatusPresentation = ParseCardStatusPresentation(query.value(9).toString())
                                        .value_or(CardStatusPresentation::PersonalListStatus);
    loaded.languageKey = NormalizeLanguageKey(query.value(10).toString());
    const auto storedPreferredTitleKey = query.value(11).toString();
    loaded.preferredTitleKey = NormalizePreferredTitleKey(storedPreferredTitleKey);
    loaded.includeAdultContent = query.value(12).toBool();
    loaded.automaticLocalFileRecognition = query.value(13).toBool();
    if (!query.value(14).toString().isEmpty()
        && !ParseStringArray(query.value(14).toString(), loaded.enabledUserLists,
                             QStringLiteral("Stored enabled user lists must be a JSON array."),
                             QStringLiteral("Stored enabled user lists must be strings."), error)) {
        return false;
    }
    loaded.lastAniListUserId = query.value(15).toLongLong();
    if (!IsSupportedPreferredTitleKey(storedPreferredTitleKey)) {
        warning = QStringLiteral(
            "Stored preferred title key '%1' is unsupported; falling back to '%2'.")
                      .arg(storedPreferredTitleKey, loaded.preferredTitleKey);
    }
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
    QString upsertQuery;
    if (!SqlQueryStore::loadSource(SqliteQueries::UpsertUserPreferences, upsertQuery, error)) {
        database_.rollback();
        return false;
    }
    if (!query.prepare(upsertQuery)) {
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
    query.bindValue(QStringLiteral(":include_adult_content"), preferences.includeAdultContent);
    query.bindValue(QStringLiteral(":automatic_local_file_recognition"), preferences.automaticLocalFileRecognition);
    query.bindValue(QStringLiteral(":enabled_user_lists"), QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(preferences.enabledUserLists)).toJson(QJsonDocument::Compact)));
    query.bindValue(QStringLiteral(":last_anilist_user_id"), preferences.lastAniListUserId);
    // library_root keeps the first folder so older builds still read a usable value.
    query.bindValue(QStringLiteral(":library_root"), preferences.libraryRoots.value(0));
    query.bindValue(QStringLiteral(":library_roots"), QString::fromUtf8(
        QJsonDocument(QJsonArray::fromStringList(preferences.libraryRoots)).toJson(QJsonDocument::Compact)));
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
