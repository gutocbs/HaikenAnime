# Local Library Scanner Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist scanner preferences and inventory candidate video files from one configured Windows directory without parsing titles or reading video content.

**Architecture:** A typed scan request flows from persisted preferences through a coordinator to one low-priority worker. The worker enumerates sequentially and writes bounded observation batches through an application repository port; only a fully successful scan reconciles unavailable paths. QML observes controller state and never performs filesystem or database work.

**Tech Stack:** C++20, Qt 6 Core/Sql/Qml/Quick, SQLite, CMake, Qt Test/CTest

**Spec:** `docs/superpowers/specs/2026-09-26-local-library-scanner-design.md`

## Global Constraints

- Scan one persisted root, defaulting to `Q:\`.
- Default extensions are `.mkv`, `.mp4`, `.avi`, `.webm`, `.m4v`, `.mov`, `.wmv`, and `.ts`; backend policy supplies choices to QML.
- Normalize extensions to lowercase with exactly one leading period; reject empty, duplicate, path-containing, or wholly empty selections.
- Enumerate recursively and sequentially without following symbolic links or junctions.
- Never open video content, hash files, parse anime filenames, or associate media in this phase.
- Use at most 200 enumerated entries per batch and wait 25 ms between batches.
- Allow exactly one active scan; run it on a low-priority worker with a worker-owned SQLite connection.
- Reconcile unavailable files only after a complete successful scan.
- Infrastructure and scan failures must not prevent the application window or existing media library from opening.
- Use English identifiers, JSON keys, and SQL names; user-facing QML copy remains Portuguese.
- Keep SQL in resource files registered through `resources/sqlite/sqlite-queries.json`.

## Review Focus

- A root that disappears between validation and traversal must fail the scan without marking previous rows unavailable; Task 4 adds this race test.
- A case-only path or extension difference on Windows must update one logical row rather than insert another; Tasks 1 and 2 pin normalization and uniqueness.
- An unreadable child directory must make the scan incomplete even when earlier batches committed; Tasks 3 and 4 verify no missing-file reconciliation.
- Repeated startup/manual triggers must never create concurrent workers or SQLite connections sharing thread ownership; Task 4 tests rejection and shutdown.
- Changing the configured root must retain the old root's inventory and scan only the newly persisted root; Tasks 2, 5, and 6 cover root-scoped identity and persisted-snapshot execution.

---

### Task 1: Scanner preferences and validation

**Files:**
- Modify: `src/application/configuration/UserPreferences.h`
- Modify: `src/application/configuration/UserPreferencesValidator.h`
- Modify: `src/application/configuration/UserPreferencesValidator.cpp`
- Modify: `src/infrastructure/configuration/JsonSettingsReader.cpp`
- Modify: `Settings.json`
- Modify: `tests/unit/UserPreferencesValidatorTests.cpp`
- Modify: `tests/unit/JsonSettingsReaderTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `QString UserPreferences::libraryRoot` and `QStringList UserPreferences::scanExtensions`.
- Produces: `QStringList NormalizeScanExtensions(const QStringList &extensions, QString &error)`; returns an empty list on invalid input and populates `error`.
- Extends: `ValidateUserPreferences(const UserPreferences &) -> UserPreferencesValidationResult`.

- [ ] **Step 1: Add failing preference tests** for the exact `Q:\` and eight-extension defaults, lowercase/dot normalization, duplicate and path-separator rejection, empty root rejection, and at least one selected extension.
- [ ] **Step 2: Run `cmake --build cmake-build-validation --target UserPreferencesValidatorTests JsonSettingsReaderTests` and the matching CTest filter**; verify assertions fail because scanner preferences are absent.
- [ ] **Step 3: Add the typed fields and `NormalizeScanExtensions`**, include them in equality/validation, and parse strict `userPreferences.library.root` and `userPreferences.library.extensions` JSON values with safe missing-object defaults.
- [ ] **Step 4: Add the exact packaged defaults to `Settings.json`** and keep malformed explicit values as errors rather than silently replacing them.
- [ ] **Step 5: Re-run both test targets** and expect all cases to pass.
- [ ] **Step 6: Commit** with `git commit -m "feat: define local library scan preferences"` using only Task 1 files.

### Task 2: Scan history and local-file repository

**Files:**
- Create: `src/domain/library/LocalFileRecord.h`
- Create: `src/application/library/ILocalFileRepository.h`
- Create: `src/infrastructure/database/SqliteLocalFileRepository.h`
- Create: `src/infrastructure/database/SqliteLocalFileRepository.cpp`
- Create: `resources/sqlite/queries/begin-library-scan.sql`
- Create: `resources/sqlite/queries/upsert-local-file.sql`
- Create: `resources/sqlite/queries/complete-library-scan.sql`
- Create: `resources/sqlite/queries/fail-library-scan.sql`
- Create: `resources/sqlite/queries/mark-local-files-unavailable.sql`
- Modify: `resources/sqlite/sqlite-queries.json`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.h`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Create: `tests/unit/SqliteLocalFileRepositoryTests.cpp`
- Modify: `tests/unit/SqliteDatabaseTests.cpp`
- Modify: `tests/unit/SqliteQueryConfigurationTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `struct LocalFileObservation { QString rootPath; QString relativePath; QString normalizedRelativePath; QString fileName; QString extension; qint64 sizeBytes; QDateTime modifiedAt; };`.
- Produces: `enum class LibraryScanStatus { Running, Succeeded, Failed, Interrupted };`.
- Produces: `ILocalFileRepository::beginScan(const QString &rootPath, qint64 &scanId, QString &error) -> bool`.
- Produces: `ILocalFileRepository::upsertBatch(qint64 scanId, const QList<LocalFileObservation> &observations, QString &error) -> bool`.
- Produces: `ILocalFileRepository::completeScan(qint64 scanId, qsizetype observedCount, QString &error) -> bool` which atomically marks unseen rows for that root unavailable and marks the scan succeeded.
- Produces: `ILocalFileRepository::failScan(qint64 scanId, LibraryScanStatus status, qsizetype observedCount, const QString &diagnostic, QString &error) -> bool` for Failed or Interrupted only.

- [ ] **Step 1: Add failing migration tests** for new `library_scans` and `local_files` tables, status checks, root-plus-normalized-path uniqueness, recognition-state defaults, and upgrade preservation of existing media/preferences rows.
- [ ] **Step 2: Run `SqliteDatabaseTests`** and confirm the new schema assertions fail.
- [ ] **Step 3: Implement the next transactional schema migration** and require every schema version through the new version before commit.
- [ ] **Step 4: Add failing query-configuration tests** requiring all five external SQL paths and rejecting an incomplete configuration.
- [ ] **Step 5: Add SQL resources, aliases, JSON configuration, and typed query-configuration members; run `SqliteQueryConfigurationTests` green.**
- [ ] **Step 6: Add failing repository tests** for begin, multi-batch upsert, case-normalized identity, duplicate physical paths, successful unavailable reconciliation, failed/interrupted scans preserving availability, root-scoped reconciliation, and transaction rollback.
- [ ] **Step 7: Implement `SqliteLocalFileRepository`** using injected external statements and bounded transactions; `completeScan` must perform reconciliation and terminal status update in one transaction.
- [ ] **Step 8: Run repository, migration, and query-configuration tests** and expect all to pass.
- [ ] **Step 9: Commit** with `git commit -m "feat: persist local library inventory"` using only Task 2 files.

### Task 3: Sequential filesystem scanner

**Files:**
- Create: `src/application/library/LocalLibraryScanTypes.h`
- Create: `src/application/library/ILocalLibraryScanner.h`
- Create: `src/infrastructure/library/IDirectoryEnumerator.h`
- Create: `src/infrastructure/library/QtDirectoryEnumerator.h`
- Create: `src/infrastructure/library/QtDirectoryEnumerator.cpp`
- Create: `src/infrastructure/library/LocalLibraryScanner.h`
- Create: `src/infrastructure/library/LocalLibraryScanner.cpp`
- Create: `tests/unit/LocalLibraryScannerTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `struct LocalLibraryScanRequest { QString rootPath; QStringList allowedExtensions; qsizetype batchSize = 200; int batchPauseMs = 25; };`.
- Produces: `struct LocalLibraryScanProgress { qsizetype visitedEntries; qsizetype candidateFiles; };`.
- Produces: `struct LocalLibraryScanResult { bool complete; bool interrupted; qsizetype candidateFiles; QString diagnostic; };`.
- Produces: `ILocalLibraryScanner::scan(const LocalLibraryScanRequest &request, const std::function<bool(const QList<LocalFileObservation> &, QString &)> &batchConsumer, const std::function<void(const LocalLibraryScanProgress &)> &progress, const std::function<bool()> &stopRequested) -> LocalLibraryScanResult`.
- Produces: `IDirectoryEnumerator`, an infrastructure seam whose production implementation wraps Qt traversal and whose test fake can report an unreadable child deterministically.
- Consumes: `LocalFileObservation` from Task 2 and normalized extensions from Task 1.

- [ ] **Step 1: Add failing scanner tests** using `QTemporaryDir` for recursion, allowed/disallowed and mixed-case extensions, hidden/temp files, symlink non-traversal, original versus normalized paths, size/time metadata, duplicate content at distinct paths, exact batch boundaries, and cooperative stop.
- [ ] **Step 2: Add a failing partial-traversal test** through an injectable directory-entry adapter that returns one unreadable child; assert `complete == false`, earlier observations remain consumable, and the diagnostic names the unreadable path.
- [ ] **Step 3: Run `LocalLibraryScannerTests`** and confirm failure because the scanner contract is absent.
- [ ] **Step 4: Implement the sequential scanner** with no file-content access, no parallelism, `QThread::msleep(request.batchPauseMs)` only between non-final batches, and a case-folded slash-normalized relative identity.
- [ ] **Step 5: Re-run `LocalLibraryScannerTests`** and expect all cases to pass deterministically.
- [ ] **Step 6: Commit** with `git commit -m "feat: scan local library directories"` using only Task 3 files.

### Task 4: Background scan coordination

**Files:**
- Create: `src/app/LocalLibraryScanCoordinator.h`
- Create: `src/app/LocalLibraryScanCoordinator.cpp`
- Create: `tests/unit/LocalLibraryScanCoordinatorTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `ILocalLibraryScanner`, `ILocalFileRepository`, and `LocalLibraryScanRequest`.
- Produces: `bool start(const LocalLibraryScanRequest &request)`; returns false when a scan is active.
- Produces: `void shutdown()`; requests stop and waits for worker completion.
- Produces signals: `started(QString rootPath)`, `progressChanged(qsizetype candidateFiles)`, `completed(qsizetype candidateFiles)`, and `failed(QString error)`.
- Contract: the worker owns its scanner and repository/SQLite connection factory products; UI-thread repository objects are never reused in the worker.

- [ ] **Step 1: Add failing coordinator tests** for successful lifecycle, throttled/coalesced progress, concurrent-start rejection, scanner failure, repository batch failure, root disappearing after start, and interrupted shutdown.
- [ ] **Step 2: Add assertions** that incomplete/failed/interrupted execution calls `failScan` and never `completeScan`, while success calls `completeScan` exactly once.
- [ ] **Step 3: Run `LocalLibraryScanCoordinatorTests`** and confirm failure before implementation.
- [ ] **Step 4: Implement the coordinator** with `QThread::create`, low priority after start, queued signals, one active execution flag, and deterministic worker-owned dependency factories.
- [ ] **Step 5: Re-run coordinator tests repeatedly** with `ctest --test-dir cmake-build-validation -R LocalLibraryScanCoordinatorTests --repeat until-fail:20 --output-on-failure`; require no timing flakes.
- [ ] **Step 6: Commit** with `git commit -m "feat: coordinate background library scans"` using only Task 4 files.

### Task 5: Settings controller and QML scanner controls

**Files:**
- Modify: `src/presentation/settings/SettingsController.h`
- Modify: `src/presentation/settings/SettingsController.cpp`
- Modify: `tests/unit/SettingsControllerTests.cpp`
- Modify: `resources/qml/SettingsScreen.qml`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Task 1 preference fields and Task 4 coordinator signals/start method.
- Produces QML properties: `libraryRoot`, `availableScanExtensions`, `selectedScanExtensions`, `scanRunning`, `scanCandidateCount`, `scanStatusMessage`, and `scanErrorMessage`.
- Produces invokables: `SetLibraryRoot(QString root)`, `SetScanExtensionEnabled(QString extension, bool enabled)`, and `ScanNow()`.
- Contract: `ScanNow()` is valid only when preferences are persisted (`!dirty`) and no scan is active.

- [ ] **Step 1: Add failing controller tests** for draft root/extensions, validation, save/discard, backend-provided extension options, dirty-state scan rejection, active-scan rejection, persisted-snapshot request construction, and coordinator status mapping.
- [ ] **Step 2: Run `SettingsControllerTests`** and verify the new cases fail.
- [ ] **Step 3: Extend the controller** while preserving atomic preference save semantics; QML's native `FolderDialog` passes the selected Windows path to `SetLibraryRoot`.
- [ ] **Step 4: Re-run controller tests** and expect them to pass.
- [ ] **Step 5: Replace the disabled library preview controls in QML** with the bound root field, native folder picker action, extension toggles, `Escanear agora`, count/status/error text, and accessible disabled-state explanation.
- [ ] **Step 6: Run QML compilation and a structural regression check** proving scanner inputs come from controller properties and no extension list is hard-coded in QML.
- [ ] **Step 7: Commit** with `git commit -m "feat: add local library scan controls"` using only Task 5 files.

### Task 6: Composition, startup scan, and shutdown

**Files:**
- Modify: `src/app/ApplicationComposition.h`
- Modify: `src/app/ApplicationComposition.cpp`
- Modify: `main.cpp`
- Create: `tests/unit/LocalLibraryScanCompositionTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 1-5 contracts.
- Produces: application-context factories that create worker-owned `LocalLibraryScanner` and `SqliteLocalFileRepository` instances from database path and external SQL resources.
- Publishes: one `LocalLibraryScanCoordinator` wired to `SettingsController`.
- Startup contract: after QML/controller initialization, start one scan from the persisted preference snapshot; failure is non-blocking and visible in settings.

- [ ] **Step 1: Add failing composition tests** proving persisted root/extensions override packaged defaults, dirty drafts are ignored, a root change preserves old-root rows, scanner query-load failure is non-fatal, and startup schedules exactly one scan.
- [ ] **Step 2: Run `LocalLibraryScanCompositionTests`** and verify failure before composition changes.
- [ ] **Step 3: Load scanner SQL resources and construct factories/coordinator** in application composition without creating cross-thread SQLite connections.
- [ ] **Step 4: Wire settings save, manual scan, startup scan, signals, and shutdown in `main.cpp`**; shutdown the scanner before database/logger destruction.
- [ ] **Step 5: Re-run composition, controller, coordinator, and repository tests** and expect all to pass.
- [ ] **Step 6: Commit** with `git commit -m "feat: run configured local library scans"` using only Task 6 files.

### Task 7: Full regression and controlled-drive verification

**Files:**
- Modify only files required to correct failures introduced by Tasks 1-6.

**Interfaces:**
- Consumes all previous task deliverables.
- Produces a verified scanner phase without title/episode recognition.

- [ ] **Step 1: Configure and build** with `cmake -S . -B cmake-build-validation -DBUILD_TESTING=ON` and `cmake --build cmake-build-validation -j 4`; require exit code 0.
- [ ] **Step 2: Run `ctest --test-dir cmake-build-validation --output-on-failure`** and require the complete suite to pass.
- [ ] **Step 3: Run `git diff --check`** and require no whitespace errors apart from explicit line-ending notices.
- [ ] **Step 4: Run a packaged startup smoke test** and require no QML, migration, query-resource, or thread-lifecycle error.
- [ ] **Step 5: Before scanning `Q:\`, manually test with a small controlled directory** containing nested allowed/disallowed files; verify count, persistence, moved/removed behavior, and disabled concurrent action.
- [ ] **Step 6: Run one monitored `Q:\` scan** only when that drive is available; confirm UI responsiveness, bounded worker behavior, terminal scan status, and that no video content is opened.
- [ ] **Step 7: Restart and verify persisted root/extensions and inventory reload**, then review `git status --short` to preserve unrelated pre-existing changes.
- [ ] **Step 8: Commit only verification-driven corrections** with `git commit -m "fix: complete local library scanner"`; skip when no corrections were necessary.
