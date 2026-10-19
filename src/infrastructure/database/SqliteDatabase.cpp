#include "SqliteDatabase.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include "../logging/AsyncLogger.h"

#include <utility>

namespace {
constexpr int LatestSchemaVersion = 26;

bool ExecuteMigrationScript(QSqlQuery &query, const QString &path, QString &error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }
    const auto statements = QString::fromUtf8(file.readAll()).split(';', Qt::SkipEmptyParts);
    for (const auto &statement : statements) {
        const auto sql = statement.trimmed();
        if (sql.isEmpty()) continue;
        if (!query.exec(sql)) {
            error = query.lastError().text();
            return false;
        }
    }
    return true;
}

/// One idempotent step of the schema chain. Every step runs on every start, in order, inside one transaction.
struct MigrationStep final {
    enum class Kind { Script, AddColumn, MarkVersion, RebuildLocalFilesWithRecognitionConstraint };
    Kind kind = Kind::Script;
    /// Script name under :/sqlite/migrations, without the .sql extension.
    QString script;
    QString table;
    QString column;
    int version = 0;
};

MigrationStep Script(QString script) {
    return {.kind = MigrationStep::Kind::Script, .script = std::move(script)};
}

/// Runs the script only when the column is still missing.
MigrationStep AddColumn(QString table, QString column, QString script) {
    return {.kind = MigrationStep::Kind::AddColumn, .script = std::move(script), .table = std::move(table),
            .column = std::move(column)};
}

MigrationStep MarkVersion(const int version) {
    return {.kind = MigrationStep::Kind::MarkVersion, .version = version};
}

const QList<MigrationStep> &MigrationSteps() {
    static const QList<MigrationStep> steps = {
        Script(QStringLiteral("001-create-initial-schema")),
        AddColumn(QStringLiteral("media"), QStringLiteral("source_removed_at"), QStringLiteral("003-add-source-removed-at-column")),
        AddColumn(QStringLiteral("media"), QStringLiteral("user_list_status"), QStringLiteral("004-add-user-list-status-column")),
        AddColumn(QStringLiteral("media"), QStringLiteral("cover_medium_url"), QStringLiteral("006-add-cover-medium-url-column")),
        AddColumn(QStringLiteral("media"), QStringLiteral("cover_large_url"), QStringLiteral("006-add-cover-large-url-column")),
        AddColumn(QStringLiteral("media"), QStringLiteral("cover_extra_large_url"), QStringLiteral("006-add-cover-extra-large-url-column")),
        MarkVersion(1), MarkVersion(2), MarkVersion(3), MarkVersion(4), MarkVersion(5), MarkVersion(6),
        Script(QStringLiteral("007-create-library-inventory")),
        MarkVersion(7),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("library_root"), QStringLiteral("008-add-library-root-preference")),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("scan_extensions"), QStringLiteral("008-add-scan-extensions-preference")),
        MarkVersion(8),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("home_sort_key"), QStringLiteral("009-add-home-sort-key-preference")),
        MarkVersion(9),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("card_status_presentation"),
                  QStringLiteral("010-add-card-status-presentation-preference")),
        MarkVersion(10),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("language_key"), QStringLiteral("011-add-language-preference")),
        MarkVersion(11),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("preferred_title_key"),
                  QStringLiteral("012-add-preferred-title-preference")),
        MarkVersion(12),
        AddColumn(QStringLiteral("media"), QStringLiteral("season"), QStringLiteral("013-add-season-to-media")),
        AddColumn(QStringLiteral("media"), QStringLiteral("season_year"), QStringLiteral("013-add-season-year-to-media")),
        AddColumn(QStringLiteral("media"), QStringLiteral("next_airing_episode"), QStringLiteral("013-add-next-airing-episode-to-media")),
        AddColumn(QStringLiteral("media"), QStringLiteral("next_airing_at"), QStringLiteral("013-add-next-airing-at-to-media")),
        AddColumn(QStringLiteral("media"), QStringLiteral("anilist_url"), QStringLiteral("013-add-anilist-url-to-media")),
        AddColumn(QStringLiteral("media"), QStringLiteral("external_links"), QStringLiteral("013-add-external-links-to-media")),
        MarkVersion(13),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("include_adult_content"),
                  QStringLiteral("014-add-adult-content-preference")),
        MarkVersion(14),
        AddColumn(QStringLiteral("media"), QStringLiteral("local_path"), QStringLiteral("015-add-local-path-to-media")),
        MarkVersion(15),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("extracted_title"), QStringLiteral("016-add-extracted-title-to-local-files")),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("media_kind"), QStringLiteral("016-add-media-kind-to-local-files")),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("season"), QStringLiteral("016-add-season-to-local-files")),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("episode"), QStringLiteral("016-add-episode-to-local-files")),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("media_id"), QStringLiteral("016-add-media-id-to-local-files")),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("recognition_diagnostic"),
                  QStringLiteral("016-add-recognition-diagnostic-to-local-files")),
        AddColumn(QStringLiteral("local_files"), QStringLiteral("recognized_at"), QStringLiteral("016-add-recognized-at-to-local-files")),
        MarkVersion(16),
        {.kind = MigrationStep::Kind::RebuildLocalFilesWithRecognitionConstraint},
        MarkVersion(17),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("automatic_local_file_recognition"),
                  QStringLiteral("018-add-automatic-local-file-recognition-preference")),
        MarkVersion(18),
        Script(QStringLiteral("019-create-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("status"), QStringLiteral("019-add-status-to-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("next_run_at"), QStringLiteral("019-add-next-run-at-to-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("safe_error_detail"),
                  QStringLiteral("019-add-safe-error-detail-to-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("confirmed_page"),
                  QStringLiteral("019-add-confirmed-page-to-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("confirmed_cursor"),
                  QStringLiteral("019-add-confirmed-cursor-to-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("priority"), QStringLiteral("019-add-priority-to-sync-task-state")),
        AddColumn(QStringLiteral("sync_task_state"), QStringLiteral("generation"), QStringLiteral("019-add-generation-to-sync-task-state")),
        MarkVersion(19),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("enabled_user_lists"),
                  QStringLiteral("020-add-enabled-user-lists-preference")),
        MarkVersion(20),
        AddColumn(QStringLiteral("anilist_pending_changes"), QStringLiteral("owner_user_id"),
                  QStringLiteral("021-add-owner-user-id-to-pending-changes")),
        MarkVersion(21),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("last_anilist_user_id"),
                  QStringLiteral("022-add-last-anilist-user-id-preference")),
        MarkVersion(22),
        AddColumn(QStringLiteral("media"), QStringLiteral("personal_list_removed_at"),
                  QStringLiteral("023-add-personal-list-removed-at-to-media")),
        MarkVersion(23),
        Script(QStringLiteral("024-create-plugin-settings")),
        MarkVersion(24),
        Script(QStringLiteral("025-create-recognition-history")),
        MarkVersion(25),
        AddColumn(QStringLiteral("user_preferences"), QStringLiteral("library_roots"), QStringLiteral("026-add-library-roots-preference")),
        MarkVersion(LatestSchemaVersion),
    };
    return steps;
}

QString MigrationScriptPath(const QString &script) {
    return QStringLiteral(":/sqlite/migrations/%1.sql").arg(script);
}

bool HasColumn(QSqlQuery &query, const QString &table, const QString &column, bool &exists, QString &error) {
    exists = false;
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        error = query.lastError().text();
        return false;
    }
    while (query.next()) exists |= query.value(1).toString() == column;
    return true;
}

// Version 17 recreates local_files so its recognition_state CHECK also accepts 'ambiguous'.
bool RebuildLocalFilesWithRecognitionConstraint(QSqlQuery &query, QString &error) {
    const bool constraintReady =
        query.exec(QStringLiteral("SELECT sql FROM sqlite_master WHERE type = 'table' AND name = 'local_files'"))
        && query.next() && query.value(0).toString().contains(QStringLiteral("'ambiguous'"));
    if (constraintReady) return true;
    for (const auto &script : {QStringLiteral("017-rename-legacy-local-files-table"),
                               QStringLiteral("017-create-local-files-with-recognition-constraint"),
                               QStringLiteral("017-copy-legacy-local-files"),
                               QStringLiteral("017-drop-legacy-local-files-table")}) {
        if (!ExecuteMigrationScript(query, MigrationScriptPath(script), error)) return false;
    }
    return true;
}

bool RunMigrationStep(QSqlQuery &query, const MigrationStep &step, QString &error) {
    switch (step.kind) {
    case MigrationStep::Kind::Script:
        return ExecuteMigrationScript(query, MigrationScriptPath(step.script), error);
    case MigrationStep::Kind::AddColumn: {
        bool exists = false;
        if (!HasColumn(query, step.table, step.column, exists, error)) return false;
        return exists || ExecuteMigrationScript(query, MigrationScriptPath(step.script), error);
    }
    case MigrationStep::Kind::MarkVersion:
        if (query.exec(QStringLiteral("INSERT OR IGNORE INTO schema_version (version) VALUES (%1)").arg(step.version))) {
            return true;
        }
        error = query.lastError().text();
        return false;
    case MigrationStep::Kind::RebuildLocalFilesWithRecognitionConstraint:
        return RebuildLocalFilesWithRecognitionConstraint(query, error);
    }
    return false;
}

// A trigger or constraint can silently skip a version insert, so every version is checked at the end.
bool AllVersionsRecorded(QSqlQuery &query) {
    return query.exec(QStringLiteral("SELECT COUNT(*) FROM schema_version WHERE version BETWEEN 1 AND %1")
                          .arg(LatestSchemaVersion))
        && query.next() && query.value(0).toInt() == LatestSchemaVersion;
}
}

SqliteDatabase::SqliteDatabase(QString databasePath)
    : connectionName_(QStringLiteral("haikenanime-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces))),
      databasePath_(std::move(databasePath)) {
    if (databasePath_.isEmpty()) {
        const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        databasePath_ = QDir(appDataPath).filePath(QStringLiteral("haikenanime.sqlite"));
    }
}

void SqliteDatabase::setLogger(AsyncLogger *logger) { logger_ = logger; }

SqliteDatabase::~SqliteDatabase() {
    close();
}

bool SqliteDatabase::open() {
    lastError_.clear();
    if (database_.isOpen()) {
        return true;
    }

    const QFileInfo fileInfo(databasePath_);
    if (!QDir().mkpath(fileInfo.absolutePath())) {
        lastError_ = QStringLiteral("Could not create the SQLite database directory '%1'.")
                         .arg(fileInfo.absolutePath());
        return false;
    }

    database_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    database_.setDatabaseName(databasePath_);
    bool opened = database_.open();
    if (!opened) {
        lastError_ = database_.lastError().text();
    }
    if (opened) {
        QSqlQuery pragma(database_);
        opened = ExecuteMigrationScript(
            pragma, QStringLiteral(":/sqlite/migrations/000-enable-foreign-keys.sql"), lastError_)
            && pragma.exec(QStringLiteral("PRAGMA busy_timeout = 10000"));
        if (!opened && lastError_.isEmpty()) lastError_ = pragma.lastError().text();
        if (!opened) {
            database_.close();
        }
    }
    if (logger_) {
        (opened ? logger_->info(LogCategory::Database, QStringLiteral("SQLite database opened: %1").arg(databasePath_))
                : logger_->error(LogCategory::Database, database_.lastError().text()));
    }
    return opened;
}

bool SqliteDatabase::migrate() {
    lastError_.clear();
    if (!database_.isOpen()) {
        lastError_ = QStringLiteral("SQLite database is not open.");
        return false;
    }

    {
        QSqlQuery pragma(database_);
        if (!pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"))) {
            lastError_ = pragma.lastError().text();
            return false;
        }
    }

    if (!database_.transaction()) {
        lastError_ = database_.lastError().text();
        return false;
    }

    QSqlQuery query(database_);
    bool stepsSucceeded = true;
    for (const auto &step : MigrationSteps()) {
        if (!RunMigrationStep(query, step, lastError_)) {
            stepsSucceeded = false;
            break;
        }
    }
    const bool versionRecorded = stepsSucceeded && AllVersionsRecorded(query);

    if (versionRecorded) {
        const bool committed = database_.commit();
        if (!committed) {
            lastError_ = database_.lastError().text();
            database_.rollback();
        }
        if (logger_) {
            committed ? logger_->info(LogCategory::Migration, QStringLiteral("SQLite migration completed."))
                      : logger_->error(LogCategory::Migration, database_.lastError().text());
        }
        return committed;
    }

    // A failed step already filled lastError_ with its own diagnostic; only the final version check needs one.
    if (stepsSucceeded) {
        lastError_ = query.lastError().text();
        if (lastError_.isEmpty()) {
            lastError_ = QStringLiteral("SQLite migration did not record every schema version from 1 through %1.")
                             .arg(LatestSchemaVersion);
        }
    }
    database_.rollback();
    if (logger_) logger_->error(LogCategory::Migration, lastError_);
    return false;
}

void SqliteDatabase::close() {
    if (database_.isValid()) {
        const auto connectionName = database_.connectionName();
        database_.close();
        database_ = {};
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool SqliteDatabase::isOpen() const {
    return database_.isOpen();
}

QString SqliteDatabase::lastError() const {
    return lastError_;
}

QString SqliteDatabase::databasePath() const {
    return databasePath_;
}

QSqlDatabase SqliteDatabase::connection() const {
    return database_;
}
