# Local Library Scanner Design

## Objective

Add the first part of local-library support for anime: scan one configured Windows directory, persist a durable inventory of candidate video files, and expose controlled scan status through the settings screen.

This phase deliberately does not parse titles or episode numbers, associate files with AniList media, or launch files and players. Those capabilities will consume the inventory in later phases.

## Scope

### Included

- One configurable library root, defaulting to `Q:\` on first run.
- A backend-provided, user-selectable set of video extensions.
- Recursive, sequential directory scanning.
- Automatic scan at application startup.
- Manual scan from the Library and Recognition settings section.
- Persistent scan history and file inventory in SQLite.
- Availability reconciliation only after a complete successful scan.
- Controlled disk usage, progress reporting, and safe shutdown.

### Deferred

- Anime-title and episode recognition.
- Anitomy integration.
- Media and episode association.
- File hashing and content-based duplicate detection.
- Native Windows file identifiers and move detection.
- Continuous filesystem watching.
- Explicit user cancellation.
- Player selection and file launching.
- Multiple configured roots.

Anitomy v1 will be introduced behind a parser interface in the recognition phase. The scanner and persistence model must not depend on Anitomy. This keeps a future migration to the upstream Anitomy v2 isolated from scanning and storage.

## Default Configuration

The existing typed `UserPreferences` model gains:

- `libraryRoot`: defaults to `Q:\`.
- `scanExtensions`: defaults to `.mkv`, `.mp4`, `.avi`, `.webm`, `.m4v`, `.mov`, `.wmv`, and `.ts`.

Extensions are stored and compared in a normalized lowercase form with one leading period. Empty entries, entries with path separators, and duplicates after normalization are invalid. At least one extension must remain enabled.

Packaged `Settings.json` supplies first-run defaults. SQLite remains authoritative after the singleton user-preference row is created. The backend supplies the available extension options and current selections to QML; QML does not define scan policy.

The initial throughput values are backend constants rather than user-facing preferences:

- At most 200 enumerated entries per work batch.
- A 25 ms cooperative pause between batches.
- One scan at a time.

These constants may become advanced settings only if real-world measurements justify exposing them.

## Architecture

### Application contracts

`ILocalLibraryScanner` starts a scan using an immutable request containing the persisted root and allowed extensions. It reports throttled progress and one terminal result.

`ILocalFileRepository` persists scan runs and file observations without exposing SQLite to application services. Its operations support beginning a scan, writing one batch, completing a scan, and failing a scan.

### Infrastructure

`LocalLibraryScanner` performs the filesystem traversal. It knows directory and file metadata but has no knowledge of AniList, anime titles, episodes, or QML.

`SqliteLocalFileRepository` implements the inventory and scan-run transactions using external SQL resources. A worker creates and owns its own named SQLite connection; a connection is never moved across threads.

### Coordination and presentation

`LocalLibraryScanCoordinator` owns worker lifecycle, rejects concurrent starts, applies the work-batch pause, throttles presentation updates, and waits for safe worker completion during application shutdown.

`SettingsController` continues to own the persisted/draft preference workflow. It exposes the configured root, allowed extension options, selected extensions, and dirty state. Scan commands always consume a persisted preference snapshot.

A focused scan presentation controller, or a clearly separated scan portion of the settings controller, exposes only scan state: idle/running/succeeded/failed, root, discovered count, status text, and error text. Filesystem traversal and SQL do not run in QML.

## Data Flow

```text
Persisted preferences
        |
        v
LocalLibraryScanCoordinator
        |
        v
LocalLibraryScanner -- batches of observations --> ILocalFileRepository
        |                                             |
        +------ throttled progress -------------------+--> settings UI
```

At startup, application composition supplies the persisted preference snapshot to the coordinator. A manual scan follows the same path. If settings contain unsaved changes, the manual scan button is disabled and explains that changes must be saved first.

## Filesystem Policy

- Traverse the configured root recursively and sequentially.
- Do not follow symbolic links or directory junctions.
- Ignore hidden directories and temporary files.
- Accept regular files only when their extension is enabled after case-insensitive normalization.
- Read only directory entries and basic metadata: original path, filename, extension, size, and last-modified time.
- Do not open video content, calculate hashes, or run recognition during scanning.
- Preserve the path spelling returned by the filesystem for display and launching.
- Maintain a separate normalized relative path for case-insensitive Windows identity comparisons.

The scanner records every candidate file selected by extension, even when later recognition cannot identify it as anime.

## Persistence Model

### `library_scans`

Each attempt stores:

- Numeric identifier.
- Root path.
- Start and finish timestamps.
- Status: running, succeeded, failed, or interrupted.
- Number of candidate files observed.
- Optional diagnostic message.

### `local_files`

Each candidate stores:

- Numeric identifier.
- Root path.
- Original relative path.
- Normalized relative path.
- Filename.
- Lowercase extension.
- Size in bytes.
- Last-modified timestamp.
- Availability flag.
- Last-seen scan identifier.
- Future-ready recognition state: unprocessed, recognized, unrecognized, or associated.

The unique identity is the root plus normalized relative path. Original and normalized paths have different responsibilities and must not overwrite each other.

## Scan Transaction Semantics

Starting a scan creates a `running` scan row. Each completed batch upserts observations and updates `last_seen_scan_id` and availability. Batch writes use bounded transactions so a large library does not hold one transaction for the entire traversal.

Only a complete successful traversal may reconcile missing files. Completion marks active rows for the same root unavailable when their `last_seen_scan_id` differs from the completed scan.

If the root is unavailable, any directory is unreadable, a repository batch fails, or shutdown interrupts traversal:

- The scan is terminally recorded as failed or interrupted.
- Successfully written observations may remain updated.
- No previous file is marked unavailable.

This asymmetry intentionally prefers stale availability over false removals.

## Moves and Duplicates

The first version does not infer file moves. After a successful scan, the old path becomes unavailable and the new path is inserted as a new active file. Filename-and-size heuristics are not sufficiently reliable, and native Windows file identifiers add platform and network-drive behavior that is outside this phase.

Every distinct path is persisted, even when multiple files have identical names, sizes, or content. Later recognition may associate multiple physical files with the same media and episode. Content hashes are excluded because they would require reading large video files and violate the initial disk-load goal.

## Controlled Resource Use

- Exactly one scanner worker may run.
- The worker runs at low thread priority.
- Traversal is sequential; there is no parallel directory enumeration or metadata fan-out.
- Work is divided into batches of at most 200 entries.
- The worker yields for 25 ms between batches and observes shutdown requests at batch boundaries.
- Repository writes are performed per batch on the worker-owned connection.
- Presentation progress is rate-limited and never emits once per discovered file.

The design controls application-generated pressure without pretending to guarantee a fixed disk bandwidth. Because file content is never read, the dominant cost remains directory enumeration and basic metadata retrieval.

## Settings Experience

The Library and Recognition section provides:

- A persisted library-root field with the native Windows folder picker.
- Backend-provided extension toggles.
- A `Scan now` action.
- Idle, running, succeeded, and failed status.
- The active root and discovered-file count while running.
- A clear message when the root is unavailable or a scan is incomplete.

The action is disabled while a scan is active or while settings have unsaved changes. Explicit cancellation is deferred; application shutdown requests cooperative stop and waits for the current batch to finish.

## Error Handling

- A missing or inaccessible root fails before traversal and preserves the previous inventory.
- An inaccessible child directory makes the scan incomplete; observations may update, but availability reconciliation is skipped.
- A file disappearing between enumeration and metadata lookup is skipped and included in the final diagnostic count.
- A database error rolls back the affected batch, fails the scan, and skips availability reconciliation.
- Scan failures are reported in settings but do not prevent the application window from opening or the existing media library from loading.
- Shutdown first requests worker stop, then waits for the batch boundary and connection closure before destroying repositories and logging.

## Validation

Automated tests use temporary directories and temporary SQLite databases. Required coverage includes:

- Recursive traversal with allowed and disallowed extensions.
- Case-insensitive extension and Windows-path normalization.
- Hidden directories, temporary files, and symbolic links.
- Distinct paths with duplicate names or content.
- A removed file across two successful scans.
- A moved file becoming one unavailable row and one new active row.
- Root failure and partial traversal without false unavailable rows.
- Deterministic batch boundaries and rejection of concurrent scans.
- Persistence and reload of `Q:\` and selected extensions.
- Startup and manual scan using persisted rather than dirty preferences.
- Cooperative shutdown during a scan.
- Migration of existing databases without losing media, cover cache, pending changes, or user preferences.
- Full build, CTest suite, QML compilation, startup smoke test, and a manual scan against a controlled test directory before using `Q:\`.

## Delivery Boundary

This phase is complete when the application can persist scanner settings, enumerate and inventory the configured directory with controlled disk pressure, report its state, and safely reconcile missing paths after successful scans. No filename parsing or AniList association is required for completion.
