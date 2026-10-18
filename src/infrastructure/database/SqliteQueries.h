#ifndef HAIKENANIME_SQLITEQUERIES_H
#define HAIKENANIME_SQLITEQUERIES_H

#include <QString>

/// Paths of the SQL statements embedded from resources/sqlite/queries (one constant per file).
namespace SqliteQueries {
inline const QString AdoptOrphanPendingChanges = QStringLiteral(":/sqlite/queries/adopt-orphan-pending-changes.sql");
inline const QString BeginLibraryScan = QStringLiteral(":/sqlite/queries/begin-library-scan.sql");
inline const QString ClearCoverCache = QStringLiteral(":/sqlite/queries/clear-cover-cache.sql");
inline const QString CompleteLibraryScan = QStringLiteral(":/sqlite/queries/complete-library-scan.sql");
inline const QString DeleteCoverCache = QStringLiteral(":/sqlite/queries/delete-cover-cache.sql");
inline const QString DeletePluginSettings = QStringLiteral(":/sqlite/queries/delete-plugin-settings.sql");
inline const QString DeleteRecognitionRecord = QStringLiteral(":/sqlite/queries/delete-recognition-record.sql");
inline const QString DeleteSyncTaskState = QStringLiteral(":/sqlite/queries/delete-sync-task-state.sql");
inline const QString DiscardRecognitionProgress = QStringLiteral(":/sqlite/queries/discard-recognition-progress.sql");
inline const QString EnqueuePendingChange = QStringLiteral(":/sqlite/queries/enqueue-pending-change.sql");
inline const QString FailLibraryScan = QStringLiteral(":/sqlite/queries/fail-library-scan.sql");
inline const QString FindQueuedRecognitionProgress = QStringLiteral(":/sqlite/queries/find-queued-recognition-progress.sql");
inline const QString InsertRecognitionRecord = QStringLiteral(":/sqlite/queries/insert-recognition-record.sql");
inline const QString MarkLocalFilesUnavailable = QStringLiteral(":/sqlite/queries/mark-local-files-unavailable.sql");
inline const QString MarkMediaPersonalListRemoved = QStringLiteral(":/sqlite/queries/mark-media-personal-list-removed.sql");
inline const QString MarkMediaSourceRemoved = QStringLiteral(":/sqlite/queries/mark-media-source-removed.sql");
inline const QString ReadActiveMediaIds = QStringLiteral(":/sqlite/queries/read-active-media-ids.sql");
inline const QString ReadAvailableEpisodeCount = QStringLiteral(":/sqlite/queries/read-available-episode-count.sql");
inline const QString ReadCatalogMediaForRecognition = QStringLiteral(":/sqlite/queries/read-catalog-media-for-recognition.sql");
inline const QString ReadCoverCache = QStringLiteral(":/sqlite/queries/read-cover-cache.sql");
inline const QString ReadLocalFileRoots = QStringLiteral(":/sqlite/queries/read-local-file-roots.sql");
inline const QString ReadLocalFilesUnderFolder = QStringLiteral(":/sqlite/queries/read-local-files-under-folder.sql");
inline const QString ReadMediaForRecognitionProgress = QStringLiteral(":/sqlite/queries/read-media-for-recognition-progress.sql");
inline const QString ReadMedia = QStringLiteral(":/sqlite/queries/read-media.sql");
inline const QString ReadNextLocalEpisode = QStringLiteral(":/sqlite/queries/read-next-local-episode.sql");
inline const QString ReadPendingChanges = QStringLiteral(":/sqlite/queries/read-pending-changes.sql");
inline const QString ReadPendingLocalFiles = QStringLiteral(":/sqlite/queries/read-pending-local-files.sql");
inline const QString ReadPendingMediaIds = QStringLiteral(":/sqlite/queries/read-pending-media-ids.sql");
inline const QString ReadPluginSettings = QStringLiteral(":/sqlite/queries/read-plugin-settings.sql");
inline const QString ReadRecognitionHistory = QStringLiteral(":/sqlite/queries/read-recognition-history.sql");
inline const QString ReadSyncTaskStates = QStringLiteral(":/sqlite/queries/read-sync-task-states.sql");
inline const QString ReadUnsentMediaIds = QStringLiteral(":/sqlite/queries/read-unsent-media-ids.sql");
inline const QString ReadUserPreferences = QStringLiteral(":/sqlite/queries/read-user-preferences.sql");
inline const QString SaveLocalFileRecognition = QStringLiteral(":/sqlite/queries/save-local-file-recognition.sql");
inline const QString SupersedePendingDeletion = QStringLiteral(":/sqlite/queries/supersede-pending-deletion.sql");
inline const QString UpdatePendingChange = QStringLiteral(":/sqlite/queries/update-pending-change.sql");
inline const QString UpdatePersonalListMedia = QStringLiteral(":/sqlite/queries/update-personal-list-media.sql");
inline const QString UpdateRecognitionRecord = QStringLiteral(":/sqlite/queries/update-recognition-record.sql");
inline const QString UpsertCoverCache = QStringLiteral(":/sqlite/queries/upsert-cover-cache.sql");
inline const QString UpsertLocalFile = QStringLiteral(":/sqlite/queries/upsert-local-file.sql");
inline const QString UpsertMedia = QStringLiteral(":/sqlite/queries/upsert-media.sql");
inline const QString UpsertPluginSettings = QStringLiteral(":/sqlite/queries/upsert-plugin-settings.sql");
inline const QString UpsertSyncTaskState = QStringLiteral(":/sqlite/queries/upsert-sync-task-state.sql");
inline const QString UpsertUserPreferences = QStringLiteral(":/sqlite/queries/upsert-user-preferences.sql");
}

#endif // HAIKENANIME_SQLITEQUERIES_H
