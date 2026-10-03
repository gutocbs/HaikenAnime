#include "SqliteDatabase.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSet>
#include <QUuid>
#include "../logging/AsyncLogger.h"

#include <utility>

namespace {
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
            pragma, QStringLiteral(":/sqlite/migrations/000-enable-foreign-keys.sql"), lastError_);
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

    if (!database_.transaction()) {
        lastError_ = database_.lastError().text();
        return false;
    }

    QSqlQuery query(database_);
    const bool schemaCreated = ExecuteMigrationScript(
        query, QStringLiteral(":/sqlite/migrations/001-create-initial-schema.sql"), lastError_);
    const bool mediaCreated = schemaCreated;
    const bool pendingChangesCreated = mediaCreated;
    const bool coverCacheCreated = pendingChangesCreated;
    const bool userPreferencesCreated = coverCacheCreated;
    bool sourceRemovalColumnExists = false;
    bool userListStatusColumnExists = false;
    bool coverMediumColumnExists = false;
    bool coverLargeColumnExists = false;
    bool coverExtraLargeColumnExists = false;
    bool seasonColumnExists = false;
    bool seasonYearColumnExists = false;
    bool nextAiringEpisodeColumnExists = false;
    bool nextAiringAtColumnExists = false;
    bool aniListUrlColumnExists = false;
    bool externalLinksColumnExists = false;
    bool localPathColumnExists = false;
    bool sourceRemovalReady = false;
    bool userListStatusReady = false;
    if (userPreferencesCreated && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-media-columns.sql"), lastError_)) {
        while (query.next()) {
            const auto column = query.value(1).toString();
            if (column == QStringLiteral("source_removed_at")) sourceRemovalColumnExists = true;
            if (column == QStringLiteral("user_list_status")) userListStatusColumnExists = true;
            if (column == QStringLiteral("cover_medium_url")) coverMediumColumnExists = true;
            if (column == QStringLiteral("cover_large_url")) coverLargeColumnExists = true;
            if (column == QStringLiteral("cover_extra_large_url")) coverExtraLargeColumnExists = true;
            if (column == QStringLiteral("season")) seasonColumnExists = true;
            if (column == QStringLiteral("season_year")) seasonYearColumnExists = true;
            if (column == QStringLiteral("next_airing_episode")) nextAiringEpisodeColumnExists = true;
            if (column == QStringLiteral("next_airing_at")) nextAiringAtColumnExists = true;
            if (column == QStringLiteral("anilist_url")) aniListUrlColumnExists = true;
            if (column == QStringLiteral("external_links")) externalLinksColumnExists = true;
            if (column == QStringLiteral("local_path")) localPathColumnExists = true;
        }
        sourceRemovalReady = sourceRemovalColumnExists
            || ExecuteMigrationScript(
                query, QStringLiteral(":/sqlite/migrations/003-add-source-removed-at.sql"), lastError_);
        userListStatusReady = userListStatusColumnExists
            || ExecuteMigrationScript(
                query, QStringLiteral(":/sqlite/migrations/004-add-user-list-status.sql"), lastError_);
    }
    const bool coverMediumReady = coverMediumColumnExists
        || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/006-add-cover-medium-url.sql"), lastError_);
    const bool coverLargeReady = coverMediumReady && (coverLargeColumnExists
        || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/006-add-cover-large-url.sql"), lastError_));
    const bool coverExtraLargeReady = coverLargeReady && (coverExtraLargeColumnExists
        || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/006-add-cover-extra-large-url.sql"), lastError_));
    const bool versionInserted = sourceRemovalReady && userListStatusReady && coverExtraLargeReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/001-record-schema-version.sql"), lastError_);
    const bool coverVersionInserted = versionInserted
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/002-record-schema-version.sql"), lastError_);
    const bool sourceRemovalVersionInserted = coverVersionInserted
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/003-record-schema-version.sql"), lastError_);
    const bool userListStatusVersionInserted = sourceRemovalVersionInserted
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/004-record-schema-version.sql"), lastError_);
    const bool userPreferencesVersionInserted = userListStatusVersionInserted
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/005-record-schema-version.sql"), lastError_);
    const bool coverVariantsVersionInserted = userPreferencesVersionInserted
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/006-record-schema-version.sql"), lastError_);
    const bool libraryScansCreated = coverVariantsVersionInserted && ExecuteMigrationScript(
        query, QStringLiteral(":/sqlite/migrations/007-create-library-inventory.sql"), lastError_);
    const bool localFilesCreated = libraryScansCreated;
    const bool inventoryVersionInserted = localFilesCreated
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/007-record-schema-version.sql"), lastError_);
    bool libraryRootExists = false;
    bool scanExtensionsExists = false;
    bool homeSortKeyExists = false;
    bool scannerPreferencesReady = false;
    if (inventoryVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-user-preference-columns.sql"), lastError_)) {
        while (query.next()) {
            const auto column = query.value(1).toString();
            if (column == QStringLiteral("library_root")) libraryRootExists = true;
            if (column == QStringLiteral("scan_extensions")) scanExtensionsExists = true;
            if (column == QStringLiteral("home_sort_key")) homeSortKeyExists = true;
        }
        scannerPreferencesReady = (libraryRootExists || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/008-add-library-root.sql"), lastError_))
            && (scanExtensionsExists || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/008-add-scan-extensions.sql"), lastError_));
    }
    const bool scannerPreferencesVersionInserted = scannerPreferencesReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/008-record-schema-version.sql"), lastError_);
    const bool homeSortKeyReady = scannerPreferencesVersionInserted && (homeSortKeyExists || ExecuteMigrationScript(
        query, QStringLiteral(":/sqlite/migrations/009-add-home-sort-key.sql"), lastError_));
    const bool homeSortKeyVersionInserted = homeSortKeyReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/009-record-schema-version.sql"), lastError_);
    bool cardStatusPresentationExists = false;
    bool cardStatusPresentationReady = false;
    if (homeSortKeyVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-user-preference-columns.sql"), lastError_)) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("card_status_presentation")) {
                cardStatusPresentationExists = true;
            }
        }
        cardStatusPresentationReady = cardStatusPresentationExists || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/010-add-card-status-presentation.sql"), lastError_);
    }
    const bool cardStatusPresentationVersionInserted = cardStatusPresentationReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/010-record-schema-version.sql"), lastError_);
    bool languageKeyExists = false;
    bool languageKeyReady = false;
    if (cardStatusPresentationVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-user-preference-columns.sql"), lastError_)) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("language_key")) languageKeyExists = true;
        }
        languageKeyReady = languageKeyExists || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/011-add-language-key.sql"), lastError_);
    }
    const bool languageVersionInserted = languageKeyReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/011-record-schema-version.sql"), lastError_);
    bool preferredTitleKeyExists = false;
    bool preferredTitleKeyReady = false;
    if (languageVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-user-preference-columns.sql"), lastError_)) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("preferred_title_key")) {
                preferredTitleKeyExists = true;
            }
        }
        preferredTitleKeyReady = preferredTitleKeyExists || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/012-add-preferred-title-key.sql"), lastError_);
    }
    const bool preferredTitleVersionInserted = preferredTitleKeyReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/012-record-schema-version.sql"), lastError_);
    const bool seasonReady = preferredTitleVersionInserted && (seasonColumnExists
        || ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/013-add-season.sql"), lastError_));
    const bool seasonYearReady = seasonReady && (seasonYearColumnExists
        || ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/013-add-season-year.sql"), lastError_));
    const bool nextAiringEpisodeReady = seasonYearReady && (nextAiringEpisodeColumnExists
        || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/013-add-next-airing-episode.sql"), lastError_));
    const bool nextAiringAtReady = nextAiringEpisodeReady && (nextAiringAtColumnExists
        || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/013-add-next-airing-at.sql"), lastError_));
    const bool aniListUrlReady = nextAiringAtReady && (aniListUrlColumnExists
        || ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/013-add-anilist-url.sql"), lastError_));
    const bool externalLinksReady = aniListUrlReady && (externalLinksColumnExists
        || ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/013-add-external-links.sql"), lastError_));
    const bool extendedMediaVersionInserted = externalLinksReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/013-record-schema-version.sql"), lastError_);
    bool includeAdultContentExists = false;
    bool adultContentReady = false;
    if (extendedMediaVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-user-preference-columns.sql"), lastError_)) {
        while (query.next()) {
            if (query.value(1).toString() == QStringLiteral("include_adult_content")) {
                includeAdultContentExists = true;
            }
        }
        adultContentReady = includeAdultContentExists || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/014-add-include-adult-content.sql"), lastError_);
    }
    const bool adultContentVersionInserted = adultContentReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/014-record-schema-version.sql"), lastError_);
    const bool localPathReady = adultContentVersionInserted && (localPathColumnExists
        || ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/015-add-local-path.sql"), lastError_));
    const bool localPathVersionInserted = localPathReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/015-record-schema-version.sql"), lastError_);
    bool recognitionFieldsReady = false;
    if (localPathVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-local-file-columns.sql"), lastError_)) {
        QSet<QString> columns;
        while (query.next()) columns.insert(query.value(1).toString());
        const auto addColumn = [&query, &columns, this](const QString &name, const QString &path) {
            if (columns.contains(name)) return true;
            if (!ExecuteMigrationScript(query, path, lastError_)) return false;
            columns.insert(name);
            return true;
        };
        recognitionFieldsReady = addColumn(QStringLiteral("extracted_title"), QStringLiteral(":/sqlite/migrations/016-add-extracted-title.sql"))
            && addColumn(QStringLiteral("media_kind"), QStringLiteral(":/sqlite/migrations/016-add-media-kind.sql"))
            && addColumn(QStringLiteral("season"), QStringLiteral(":/sqlite/migrations/016-add-season.sql"))
            && addColumn(QStringLiteral("episode"), QStringLiteral(":/sqlite/migrations/016-add-episode.sql"))
            && addColumn(QStringLiteral("media_id"), QStringLiteral(":/sqlite/migrations/016-add-media-id.sql"))
            && addColumn(QStringLiteral("recognition_diagnostic"), QStringLiteral(":/sqlite/migrations/016-add-recognition-diagnostic.sql"))
            && addColumn(QStringLiteral("recognized_at"), QStringLiteral(":/sqlite/migrations/016-add-recognized-at.sql"));
    }
    const bool recognitionVersionInserted = recognitionFieldsReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/016-record-schema-version.sql"), lastError_);
    bool recognitionStateConstraintReady = false;
    if (recognitionVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-local-files-schema.sql"), lastError_) && query.next()) {
        recognitionStateConstraintReady = query.value(0).toString().contains(QStringLiteral("'ambiguous'"));
    }
    if (recognitionVersionInserted && !recognitionStateConstraintReady) {
        const bool legacyLocalFilesRenamed = ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/017-rename-legacy-local-files.sql"), lastError_);
        const bool localFilesRecreated = legacyLocalFilesRenamed && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/017-create-local-files.sql"), lastError_);
        const bool localFilesCopied = localFilesRecreated && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/017-copy-local-files.sql"), lastError_);
        recognitionStateConstraintReady = localFilesCopied && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/017-drop-legacy-local-files.sql"), lastError_);
    }
    const bool recognitionStateVersionInserted = recognitionStateConstraintReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/017-record-schema-version.sql"), lastError_);
    bool automaticRecognitionColumnExists = false;
    if (recognitionStateVersionInserted && ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/000-read-user-preference-columns.sql"), lastError_)) {
        while (query.next()) automaticRecognitionColumnExists |= query.value(1).toString() == QStringLiteral("automatic_local_file_recognition");
    }
    const bool automaticRecognitionReady = recognitionStateVersionInserted && (automaticRecognitionColumnExists
        || ExecuteMigrationScript(
            query, QStringLiteral(":/sqlite/migrations/018-add-automatic-local-file-recognition.sql"), lastError_));
    const bool automaticRecognitionVersionInserted = automaticRecognitionReady
        && ExecuteMigrationScript(query, QStringLiteral(":/sqlite/migrations/018-record-schema-version.sql"), lastError_);
    const bool versionQueried = automaticRecognitionVersionInserted && ExecuteMigrationScript(
        query, QStringLiteral(":/sqlite/migrations/000-read-schema-version-status.sql"), lastError_);
    const bool versionRecorded = versionQueried && query.next() && query.value(0).toBool()
        && query.value(1).toBool() && query.value(2).toBool() && query.value(3).toBool()
        && query.value(4).toBool() && query.value(5).toBool() && query.value(6).toBool()
        && query.value(7).toBool() && query.value(8).toBool() && query.value(9).toBool()
        && query.value(10).toBool() && query.value(11).toBool() && query.value(12).toBool()
        && query.value(13).toBool() && query.value(14).toBool() && query.value(15).toBool()
        && query.value(16).toBool() && query.value(17).toBool();

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

    lastError_ = query.lastError().text();
    if (lastError_.isEmpty()) {
        lastError_ = QStringLiteral("SQLite migration versions 1 through 15 were not recorded.");
    }
    database_.rollback();
    if (logger_) logger_->error(LogCategory::Migration, query.lastError().text());
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
