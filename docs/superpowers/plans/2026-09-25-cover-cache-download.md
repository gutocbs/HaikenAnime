# Persistent Cover Cache and Download Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Download AniList covers lazily for the visible library window, persist validated files for offline use, replace changed covers safely, and update only the affected QML card.

**Architecture:** A presentation-owned visible window feeds an application `CoverDownloadCoordinator`. The coordinator schedules bounded work through an injected downloader and persists metadata/files through infrastructure contracts; QML renders only a prepared local `coverSource`. SQLite stores relative cache metadata, while validated image files live under `QStandardPaths::AppDataLocation/covers`.

**Tech Stack:** C++20, Qt 6 Core/Network/Sql/Gui/Test, QML `GridView`, SQLite, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-09-25-cover-cache-download-design.md`

## Global Constraints

- Preserve the current `domain -> application -> infrastructure -> presentation` dependency direction; QML never owns HTTP, filesystem, cache, or retry rules.
- Use English identifiers and JSON keys; public methods follow the project's requested uppercase-method convention for new cover components.
- Keep SQL and GraphQL in external resource files with explicit `QT_RESOURCE_ALIAS` values.
- Use `medium` as the initial cover quality; accept `medium`, `large`, and `extraLarge` settings.
- Keep at most three downloads active and at most two retries for timeout, HTTP `408`, `429`, and `5xx`; honor `Retry-After`.
- Never delete the last valid cover until its replacement is validated, published, persisted, and exposed to the model.
- Use temporary directories and temporary databases in tests; never touch the user's application data.
- Do not redesign the Home screen; preserve two/three-column behavior and the existing information architecture.
- Do not implement local resizing or recompression in this delivery.
- Preserve the existing uncommitted AniList field-contract changes when executing this plan.

## Review Focus

- A cache row may point to a file deleted after startup; the visible request must fall back and enqueue exactly one replacement.
- Manual cleanup may race with a late network completion; a stale generation must not recreate cache state or a visible cover.
- A URL or quality change may fail; the previous validated cover must remain visible and on disk.
- Rapid scrolling may repeatedly submit overlapping windows; active/queued requests must remain deduplicated and bounded.
- A server may send HTML, an oversized body, or corrupt bytes with an image MIME; validation must reject it without publishing.

---

## File Structure

### New application files

- `src/application/covers/CoverQuality.h`: accepted AniList cover variants and string conversion.
- `src/application/covers/CoverSettings.h`: validated runtime policy values.
- `src/application/covers/CoverCacheEntry.h`: persisted cache metadata.
- `src/application/covers/CoverDownloadTypes.h`: request, priority, failure category, and result values.
- `src/application/covers/ICoverCacheRepository.h`: metadata persistence contract.
- `src/application/covers/ICoverFileStore.h`: file existence, publication, removal, and clearing contract.
- `src/application/covers/ICoverDownloader.h`: cancellable asynchronous transport contract.
- `src/application/covers/CoverDownloadCoordinator.h/.cpp`: visible-window scheduling, deduplication, retry, generation, and completion.

### New infrastructure files

- `src/infrastructure/database/SqliteCoverCacheRepository.h/.cpp`: `cover_cache` persistence.
- `src/infrastructure/covers/CoverFileStore.h/.cpp`: temporary files, `QImageReader` validation, atomic publication, and removal.
- `src/infrastructure/covers/QtCoverDownloader.h/.cpp`: Qt Network implementation and worker-thread ownership.
- `resources/sqlite/queries/read-cover-cache.sql`
- `resources/sqlite/queries/upsert-cover-cache.sql`
- `resources/sqlite/queries/delete-cover-cache.sql`
- `resources/sqlite/queries/clear-cover-cache.sql`
- `resources/images/cover-placeholder.svg`: packaged fallback image.

### Existing files modified

- `Settings.json`, `src/application/configuration/Settings.h`, `src/infrastructure/configuration/JsonSettingsReader.cpp`: cover settings.
- `resources/anilist/queries/media-page.graphql`, `src/infrastructure/anilist/AniListMediaDto.h`, `src/infrastructure/anilist/AniListMediaMapper.cpp`: select a configured cover variant without losing the remote URL contract.
- `src/infrastructure/database/SqliteDatabase.cpp`: migration version 2 and `cover_cache` table.
- `resources/sqlite/sqlite-queries.json`, `src/infrastructure/database/SqliteQueryConfiguration.h/.cpp`: external cache query paths.
- `src/presentation/home/HomeScreenController.h/.cpp`: local cover state, visible window, failures, and per-row updates.
- `resources/qml/Home.qml`, `resources/qml/MediaCard.qml`: virtualized grid and local image rendering.
- `src/app/ApplicationComposition.h/.cpp`, `main.cpp`: cover service composition, signal wiring, and shutdown.
- `CMakeLists.txt`: sources, resources, test targets, and Qt Gui/Network/Sql links.
- `anilist.md`, `planning.md`: implemented decisions and remaining settings-screen work.

### New tests

- `tests/unit/CoverSettingsTests.cpp`
- `tests/unit/SqliteCoverCacheRepositoryTests.cpp`
- `tests/unit/CoverFileStoreTests.cpp`
- `tests/unit/CoverDownloadCoordinatorTests.cpp`
- `tests/unit/QtCoverDownloaderTests.cpp`
- updates to `tests/unit/HomeScreenControllerTests.cpp` and `tests/unit/SqliteDatabaseTests.cpp`.

---

### Task 1: Cover settings and AniList variants

**Files:**
- Create: `src/application/covers/CoverQuality.h`
- Create: `src/application/covers/CoverSettings.h`
- Create: `tests/unit/CoverSettingsTests.cpp`
- Modify: `Settings.json`
- Modify: `src/application/configuration/Settings.h`
- Modify: `src/infrastructure/configuration/JsonSettingsReader.cpp`
- Modify: `tests/unit/JsonSettingsReaderTests.cpp`
- Modify: `resources/anilist/queries/media-page.graphql`
- Modify: `src/infrastructure/anilist/AniListMediaDto.h`
- Modify: `src/infrastructure/anilist/AniListMediaMapper.cpp`
- Modify: `tests/unit/AniListGraphQlParsingTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `enum class CoverQuality { Medium, Large, ExtraLarge };`
- Produces: `QString CoverQualityName(CoverQuality quality);`
- Produces: `std::optional<CoverQuality> ParseCoverQuality(const QString &value);`
- Produces: `CoverSettings Settings::covers` with defaults `Medium`, `3`, `30000`, `2`, `1000`, `5242880`, `32`, `4096`, and `300000` milliseconds cooldown.
- Produces: `AniListMediaDto::coverImages` containing all three remote variants so configuration changes do not require changing the query.

- [ ] **Step 1: Write failing settings and mapping tests**

Add table-driven assertions equivalent to:

```cpp
QCOMPARE(ParseCoverQuality(QStringLiteral("medium")), CoverQuality::Medium);
QCOMPARE(ParseCoverQuality(QStringLiteral("large")), CoverQuality::Large);
QCOMPARE(ParseCoverQuality(QStringLiteral("extraLarge")), CoverQuality::ExtraLarge);
QVERIFY(!ParseCoverQuality(QStringLiteral("original")).has_value());

QCOMPARE(settings.covers.quality, CoverQuality::Large);
QCOMPARE(settings.covers.maxConcurrentDownloads, 2);
QCOMPARE(dto.coverImages.medium, QStringLiteral("https://img/medium.jpg"));
QCOMPARE(dto.coverImages.large, QStringLiteral("https://img/large.jpg"));
QCOMPARE(dto.coverImages.extraLarge, QStringLiteral("https://img/xl.jpg"));
```

Use a GraphQL payload whose `coverImage` contains all three fields and verify that a null variant remains empty.

- [ ] **Step 2: Run RED tests**

Run:

```powershell
cmake --build cmake-build-validation --target CoverSettingsTests JsonSettingsReaderTests AniListGraphQlParsingTests --parallel 2
ctest --test-dir cmake-build-validation -R "^(CoverSettingsTests|JsonSettingsReaderTests|AniListGraphQlParsingTests)$" --output-on-failure
```

Expected: compilation or assertions fail because cover settings and three variants do not exist.

- [ ] **Step 3: Implement settings and variant mapping**

Define:

```cpp
struct CoverSettings {
    CoverQuality quality = CoverQuality::Medium;
    int maxConcurrentDownloads = 3;
    int timeoutMs = 30000;
    int maxRetries = 2;
    int retryDelayMs = 1000;
    int maxResponseBytes = 5 * 1024 * 1024;
    int minDimension = 32;
    int maxDimension = 4096;
    int failureCooldownMs = 5 * 60 * 1000;
};

struct AniListCoverImagesDto {
    QString medium;
    QString large;
    QString extraLarge;
};
```

Validate positive limits, nonnegative retries/delays, `minDimension <= maxDimension`, and a recognized quality. Update GraphQL to request `medium`, `large`, and `extraLarge`. Add a mapper helper that selects a URL by `CoverQuality`, falling back from `extraLarge` to `large`, then `medium`, without silently upgrading `medium` to a larger download.

- [ ] **Step 4: Run GREEN tests**

Run the two focused targets and tests from Step 2. Expected: PASS.

- [ ] **Step 5: Commit**

```powershell
git add Settings.json CMakeLists.txt resources/anilist/queries/media-page.graphql src/application/configuration src/application/covers src/infrastructure/anilist tests/unit/CoverSettingsTests.cpp tests/unit/JsonSettingsReaderTests.cpp tests/unit/AniListGraphQlParsingTests.cpp
git commit -m "feat: configure AniList cover variants"
```

### Task 2: SQLite cover metadata repository

**Files:**
- Create: `src/application/covers/CoverCacheEntry.h`
- Create: `src/application/covers/ICoverCacheRepository.h`
- Create: `src/infrastructure/database/SqliteCoverCacheRepository.h`
- Create: `src/infrastructure/database/SqliteCoverCacheRepository.cpp`
- Create: `resources/sqlite/queries/read-cover-cache.sql`
- Create: `resources/sqlite/queries/upsert-cover-cache.sql`
- Create: `resources/sqlite/queries/delete-cover-cache.sql`
- Create: `resources/sqlite/queries/clear-cover-cache.sql`
- Create: `tests/unit/SqliteCoverCacheRepositoryTests.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.h`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.cpp`
- Modify: `resources/sqlite/sqlite-queries.json`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `CoverQuality` and `CoverQualityName` from Task 1.
- Produces:

```cpp
struct CoverCacheEntry {
    int mediaId = 0;
    QString remoteUrl;
    CoverQuality quality = CoverQuality::Medium;
    QString relativePath;
    QString mimeType;
    qint64 byteSize = 0;
    QString etag;
    QString lastModified;
    QDateTime validatedAt;
};

class ICoverCacheRepository {
public:
    virtual ~ICoverCacheRepository() = default;
    virtual bool ReadAll(QHash<int, CoverCacheEntry> &entries, QString &error) = 0;
    virtual bool Upsert(const CoverCacheEntry &entry, QString &error) = 0;
    virtual bool Remove(int mediaId, QString &error) = 0;
    virtual bool Clear(QString &error) = 0;
};
```

- [ ] **Step 1: Write failing migration and repository tests**

Use `QTemporaryDir` and a temporary SQLite database. Assert migration version 2, foreign-key cascade, relative-path round trip, replacement of a URL/quality row, `Remove`, and `Clear`. Include a media deletion assertion:

```cpp
QVERIFY(repository.Upsert(entry, error));
QVERIFY(deleteMedia.exec(QStringLiteral("DELETE FROM media WHERE id = 42")));
QVERIFY(repository.ReadAll(entries, error));
QVERIFY(!entries.contains(42));
```

- [ ] **Step 2: Run RED tests**

```powershell
cmake --build cmake-build-validation --target SqliteDatabaseTests SqliteCoverCacheRepositoryTests --parallel 2
ctest --test-dir cmake-build-validation -R "^(SqliteDatabaseTests|SqliteCoverCacheRepositoryTests)$" --output-on-failure
```

Expected: missing table/repository failures.

- [ ] **Step 3: Implement migration and repository**

Migration version 2 creates:

```sql
CREATE TABLE cover_cache (
    media_id INTEGER PRIMARY KEY,
    remote_url TEXT NOT NULL,
    quality TEXT NOT NULL,
    relative_path TEXT NOT NULL,
    mime_type TEXT NOT NULL,
    byte_size INTEGER NOT NULL,
    etag TEXT NOT NULL DEFAULT '',
    last_modified TEXT NOT NULL DEFAULT '',
    validated_at TEXT NOT NULL,
    FOREIGN KEY(media_id) REFERENCES media(id) ON DELETE CASCADE
);
```

Reject absolute paths in `Upsert`. Parse `validated_at` as UTC ISO 8601 and fail with an explicit error for unsupported quality values.

- [ ] **Step 4: Run GREEN tests**

Run Step 2 commands. Expected: PASS.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt resources/sqlite src/application/covers src/infrastructure/database tests/unit/SqliteDatabaseTests.cpp tests/unit/SqliteCoverCacheRepositoryTests.cpp
git commit -m "feat: persist cover cache metadata"
```

### Task 3: Validated atomic cover file store

**Files:**
- Create: `src/application/covers/ICoverFileStore.h`
- Create: `src/infrastructure/covers/CoverFileStore.h`
- Create: `src/infrastructure/covers/CoverFileStore.cpp`
- Create: `tests/unit/CoverFileStoreTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `CoverSettings`, media ID, remote URL, temporary path, and response MIME.
- Produces:

```cpp
struct PublishedCover {
    QString relativePath;
    QString mimeType;
    qint64 byteSize = 0;
};

class ICoverFileStore {
public:
    virtual ~ICoverFileStore() = default;
    virtual bool Exists(const QString &relativePath) const = 0;
    virtual bool Publish(int mediaId, const QString &remoteUrl,
                         const QString &temporaryPath, const QString &responseMime,
                         PublishedCover &result, QString &error) = 0;
    virtual bool Remove(const QString &relativePath, QString &error) = 0;
    virtual bool Clear(QString &error) = 0;
    virtual bool RemoveOrphans(const QSet<QString> &referencedPaths,
                               int maxFiles, int &removedFiles, QString &error) = 0;
    virtual QString AbsolutePath(const QString &relativePath) const = 0;
};
```

- [ ] **Step 1: Write failing file validation tests**

Generate a 2x2 PNG with `QImage::save`, plus empty, text/HTML, truncated, oversized, too-small, and dimension-bomb fixtures in `QTemporaryDir`. Assert deterministic filename, relative path, preserved prior file on failure, no published output for invalid inputs, and orphan cleanup removing no more than `maxFiles` while preserving every referenced path.

- [ ] **Step 2: Run RED test**

```powershell
cmake --build cmake-build-validation --target CoverFileStoreTests --parallel 2
ctest --test-dir cmake-build-validation -R "^CoverFileStoreTests$" --output-on-failure
```

Expected: missing file-store implementation.

- [ ] **Step 3: Implement validation and atomic publication**

Use `QImageReader::canRead()`, `QImageReader::size()`, `QMimeDatabase`, `QCryptographicHash::Sha256`, `QSaveFile` or same-directory rename, and configured byte/dimension limits. Permit PNG, JPEG, WebP, and GIF only. Never delete an existing destination before the validated replacement is ready. `RemoveOrphans` scans at most one bounded batch per call so it cannot turn startup into an unbounded directory walk.

- [ ] **Step 4: Run GREEN test**

Run Step 2 command. Expected: PASS.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/application/covers/ICoverFileStore.h src/infrastructure/covers tests/unit/CoverFileStoreTests.cpp
git commit -m "feat: validate and publish cover files"
```

### Task 4: Asynchronous downloader and bounded coordinator

**Files:**
- Create: `src/application/covers/CoverDownloadTypes.h`
- Create: `src/application/covers/ICoverDownloader.h`
- Create: `src/application/covers/CoverDownloadCoordinator.h`
- Create: `src/application/covers/CoverDownloadCoordinator.cpp`
- Create: `src/infrastructure/covers/QtCoverDownloader.h`
- Create: `src/infrastructure/covers/QtCoverDownloader.cpp`
- Create: `tests/unit/CoverDownloadCoordinatorTests.cpp`
- Create: `tests/unit/QtCoverDownloaderTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 1-3 contracts.
- Produces:

```cpp
enum class CoverPriority { Visible, Prefetch };
enum class CoverState { Missing, Queued, Downloading, Available, Failed };
enum class CoverFailureCategory { Transport, HttpTemporary, HttpPermanent, InvalidContent, FileSystem, Persistence, Cancelled };

struct CoverRequest {
    int mediaId = 0;
    QUrl remoteUrl;
    CoverQuality quality = CoverQuality::Medium;
    CoverPriority priority = CoverPriority::Prefetch;
    quint64 generation = 0;
};

struct CoverDownloadResult {
    CoverRequest request;
    bool succeeded = false;
    QString temporaryPath;
    QString mimeType;
    QString etag;
    QString lastModified;
    CoverFailureCategory failure = CoverFailureCategory::Transport;
    int httpStatus = 0;
    int retryAfterMs = 0;
    QString error;
};

class ICoverDownloader {
public:
    using Completion = std::function<void(CoverDownloadResult)>;
    virtual ~ICoverDownloader() = default;
    virtual quint64 Start(const CoverRequest &request, Completion completion) = 0;
    virtual void Cancel(quint64 requestId) = 0;
};
```

`CoverDownloadCoordinator` exposes `RequestWindow(QList<CoverRequest> visible, QList<CoverRequest> prefetch)`, `ReportMissingFile(int mediaId)`, `Clear()`, and signals/callbacks `CoverAvailable(int, QString)`, `CoverStateChanged(int, CoverState)`, and `ClearCompleted()`.

- [ ] **Step 1: Write failing scheduler tests**

Use a controllable fake downloader and fake repositories. Assert three active requests maximum, visible-before-prefetch order, overlap deduplication, queued work removal, two retries only for temporary categories, `Retry-After` delay selection, cooldown, old-cover preservation, and stale-generation discard after `Clear()`.

Include the review-focus race:

```cpp
coordinator.RequestWindow({request}, {});
const auto staleCompletion = downloader.TakeCompletion(request.mediaId);
coordinator.Clear();
staleCompletion(successResult);
QVERIFY(!cache.entries.contains(request.mediaId));
QVERIFY(!fileStore.published.contains(request.mediaId));
```

- [ ] **Step 2: Run RED coordinator tests**

```powershell
cmake --build cmake-build-validation --target CoverDownloadCoordinatorTests --parallel 2
ctest --test-dir cmake-build-validation -R "^CoverDownloadCoordinatorTests$" --output-on-failure
```

Expected: missing coordinator and interfaces.

- [ ] **Step 3: Implement the coordinator minimally**

Maintain visible/prefetch queues, a key of `(mediaId, remoteUrl, quality)`, active request map, retry attempt count, failure cooldown timestamps, and cache generation. Publish through `ICoverFileStore`, persist through `ICoverCacheRepository`, notify availability, then remove a superseded old file.

- [ ] **Step 4: Run GREEN coordinator tests**

Run Step 2 command. Expected: PASS.

- [ ] **Step 5: Write failing Qt transport tests**

Use a local `QTcpServer` fixture to return controlled `200`, `404`, `429` with `Retry-After`, `500`, timeout, and oversized-body responses. Assert classification, header capture, maximum-body abort, and cancellation callback exactly once.

- [ ] **Step 6: Implement `QtCoverDownloader`**

Create the `QNetworkAccessManager` in the downloader's worker thread. Stream each reply to a same-directory temporary file, abort when bytes exceed `maxResponseBytes`, use a single-shot timeout, and remove partial files on failure/cancellation.

- [ ] **Step 7: Run downloader and coordinator tests**

```powershell
cmake --build cmake-build-validation --target CoverDownloadCoordinatorTests QtCoverDownloaderTests --parallel 2
ctest --test-dir cmake-build-validation -R "^(CoverDownloadCoordinatorTests|QtCoverDownloaderTests)$" --output-on-failure
```

Expected: PASS.

- [ ] **Step 8: Commit**

```powershell
git add CMakeLists.txt src/application/covers src/infrastructure/covers tests/unit/CoverDownloadCoordinatorTests.cpp tests/unit/QtCoverDownloaderTests.cpp
git commit -m "feat: schedule bounded cover downloads"
```

### Task 5: Per-item Home model integration

**Files:**
- Modify: `src/presentation/home/HomeScreenController.h`
- Modify: `src/presentation/home/HomeScreenController.cpp`
- Modify: `tests/unit/HomeScreenControllerTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `CoverDownloadCoordinator` outcomes and `CoverQuality` URL selection.
- Produces: roles `RemoteCoverUrlRole`, `CoverSourceRole`, `CoverStateRole` with names `remoteCoverUrl`, `coverSource`, and `coverState`.
- Produces: `Q_INVOKABLE void RequestCoverWindow(int firstVisibleIndex, int lastVisibleIndex, int prefetchCount);`
- Produces: `Q_INVOKABLE void ReportCoverLoadFailure(int mediaId);`
- Produces: `Q_INVOKABLE void ClearCoverCache();`
- Produces: `HomeMediaModel::UpdateCover(int mediaId, QString source, CoverState state)` returning `bool`.

- [ ] **Step 1: Write failing model/controller tests**

Inject a fake cover coordinator. Assert cached local source or placeholder at reload, visible and next-page request construction, one-row `dataChanged`, no `modelReset`, missing-file report for one media, failed download retaining an old source, and cleanup changing all rows to placeholder without changing remote URLs.

Use spies:

```cpp
QSignalSpy changed(controller.mediaModel(), &QAbstractItemModel::dataChanged);
QSignalSpy reset(controller.mediaModel(), &QAbstractItemModel::modelReset);
fakeCoordinator.Complete(42, QStringLiteral("file:///covers/42.jpg"));
QCOMPARE(changed.count(), 1);
QCOMPARE(reset.count(), 0);
```

- [ ] **Step 2: Run RED test**

```powershell
cmake --build cmake-build-validation --target HomeScreenControllerTests --parallel 2
ctest --test-dir cmake-build-validation -R "^HomeScreenControllerTests$" --output-on-failure
```

Expected: missing roles and invokables.

- [ ] **Step 3: Implement per-item presentation state**

Store cover presentation state alongside each `Media` row, provide a packaged placeholder URL, clamp/validate requested indexes, derive prefetch indexes without exceeding `rowCount`, and invoke the coordinator without blocking.

- [ ] **Step 4: Run GREEN test**

Run Step 2 command. Expected: PASS.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/presentation/home tests/unit/HomeScreenControllerTests.cpp
git commit -m "feat: update downloaded covers per media"
```

### Task 6: Virtualized QML grid and placeholder

**Files:**
- Create: `resources/images/cover-placeholder.svg`
- Modify: `resources/qml/Home.qml`
- Modify: `resources/qml/MediaCard.qml`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Task 5 QML roles and invokables.
- Produces: a `GridView` preserving the current card geometry and reporting visible ranges after layout/scroll settles.

- [ ] **Step 1: Add the placeholder resource and bind a real `Image`**

`MediaCard` gains `property url coverSource` and `property int mediaId`. The cover area contains:

```qml
Image {
    anchors.fill: parent
    source: card.coverSource
    asynchronous: true
    fillMode: Image.PreserveAspectCrop
    sourceSize.width: 140
    sourceSize.height: 200
    onStatusChanged: {
        if (status === Image.Error)
            card.coverLoadFailed(card.mediaId)
    }
}
```

Expose `signal coverLoadFailed(int mediaId)` and connect it to `controller.ReportCoverLoadFailure(mediaId)` in the delegate.

- [ ] **Step 2: Replace `GridLayout + Repeater` with `GridView`**

Preserve two columns below 760 px and three at/above 760 px. Set `cellWidth`, `cellHeight: 154`, clipping, and a one-page `cacheBuffer`. Add a 50 ms debounce timer that calculates visible first/last indexes with `indexAt()` and calls:

```qml
controller.RequestCoverWindow(firstIndex, lastIndex, visibleItemCount)
```

Trigger it on `contentY`, width, height, count, and model-reset changes; do not invoke it once per delegate.

- [ ] **Step 3: Build QML and run presentation tests**

```powershell
cmake --build cmake-build-validation --target HaikenAnime HomeScreenControllerTests --parallel 2
ctest --test-dir cmake-build-validation -R "^HomeScreenControllerTests$" --output-on-failure
```

Expected: build and test PASS with no QML compilation errors.

- [ ] **Step 4: Visually inspect wide and narrow layouts**

Run the deployed executable. Verify three columns at wide size, two at narrow size, scrolling, preserved card spacing, placeholder display, incremental replacement, and no selection/scroll reset when one cover changes. Record any environment limitation instead of substituting compilation for visual validation.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt resources/images/cover-placeholder.svg resources/qml/Home.qml resources/qml/MediaCard.qml
git commit -m "feat: virtualize cover loading in home grid"
```

### Task 7: Composition, cleanup, and shutdown

**Files:**
- Modify: `src/app/ApplicationComposition.h`
- Modify: `src/app/ApplicationComposition.cpp`
- Modify: `main.cpp`
- Create: `tests/unit/CoverCompositionTests.cpp`
- Modify: `tests/unit/InitialSyncCoordinatorTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: concrete repository, file store, downloader, coordinator, settings, and controller interfaces from Tasks 1-6.
- Produces: `ApplicationContext::coverCoordinator` with lifetime longer than `HomeScreenController`; deterministic `Stop()` that cancels work, drains callbacks, and joins its worker thread before database/logger destruction.

- [ ] **Step 1: Write failing composition and shutdown tests**

Create the application context with temporary paths/fakes. Assert invalid cover settings fall back or surface the documented configuration error without preventing the window, downloader objects live in the worker thread, and shutdown during an active request finishes within the test timeout without `QThread: Destroyed while thread is still running`.

- [ ] **Step 2: Run RED tests**

```powershell
cmake --build cmake-build-validation --target CoverCompositionTests InitialSyncCoordinatorTests --parallel 2
ctest --test-dir cmake-build-validation -R "^(CoverCompositionTests|InitialSyncCoordinatorTests)$" --output-on-failure
```

Expected: missing composition/lifecycle behavior.

- [ ] **Step 3: Compose services and lifecycle**

Construct the cover worker first, then create its SQLite connection, repository, file store, coordinator, and `QtCoverDownloader` inside that worker thread. Connect coordinator outcomes to `HomeScreenController` with queued connections, schedule bounded orphan-maintenance batches after startup, and stop covers before closing SQLite or the logger. Cover initialization failure must degrade to placeholders and an explicit nonfatal status; it must not prevent the main window from opening.

- [ ] **Step 4: Run GREEN tests**

Run Step 2 commands. Expected: PASS.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt main.cpp src/app src/application/covers src/infrastructure/covers tests/unit/CoverCompositionTests.cpp tests/unit/InitialSyncCoordinatorTests.cpp
git commit -m "feat: compose cover cache lifecycle"
```

### Task 8: Documentation and complete validation

**Files:**
- Modify: `anilist.md`
- Modify: `planning.md`
- Modify: `review.md` only if validation exposes a concrete deferred debt.

**Interfaces:**
- Consumes: all prior tasks.
- Produces: current project documentation and a fully verified delivery report.

- [ ] **Step 1: Update living documentation**

Record implemented quality selection, persistent offline cache, visible-window scheduling, replacement order, lazy missing-file recovery, cleanup behavior, and exact remaining work for the settings screen. Do not mark local recompression or full-library offline prefetch as implemented.

- [ ] **Step 2: Run focused cover tests**

```powershell
ctest --test-dir cmake-build-validation -R "(Cover|HomeScreenController|JsonSettingsReader|SqliteDatabase)" --output-on-failure
```

Expected: all selected tests PASS.

- [ ] **Step 3: Run complete build**

```powershell
cmake --build cmake-build-validation --parallel 2
```

Expected: exit code 0. If `HaikenAnime.exe` is locked, report the running process as the blocker rather than changing source.

- [ ] **Step 4: Run the full suite**

```powershell
ctest --test-dir cmake-build-validation --output-on-failure
```

Expected: 100% tests pass with zero failures.

- [ ] **Step 5: Verify source diff and runtime shutdown**

```powershell
git diff --check
```

Launch the application, allow at least one cover request to start, close the window, and confirm a normal exit with no thread-destruction warning. Repeat offline with a cached cover and confirm the local image remains visible.

- [ ] **Step 6: Commit documentation**

```powershell
git add anilist.md planning.md review.md
git commit -m "docs: record cover cache behavior"
```

- [ ] **Step 7: Request final review**

Review the complete branch against `docs/superpowers/specs/2026-09-25-cover-cache-download-design.md`, with special attention to the five Review Focus cases, thread ownership, deletion order, and whether QML remained presentation-only. Fix Critical and Important findings, then rerun Steps 2-5 before reporting completion.
