#include "SqliteUserPreferencesRepository.h"

#include <QSqlError>
#include <QSqlQuery>
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
    const auto validation = ValidateUserPreferences(loaded);
    if (!validation.valid) {
        error = validation.error;
        return false;
    }
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
    if (!query.exec() || !database_.commit()) {
        error = query.lastError().text();
        if (error.isEmpty()) error = database_.lastError().text();
        database_.rollback();
        return false;
    }
    return true;
}
