# AniList Authoritative Snapshot Reconciliation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Logically remove media absent from a complete AniList snapshot and reactivate returning media without losing local data or covers.

**Architecture:** `AniListSyncService` will explicitly invoke an `IMediaSnapshotReconciler` only after a successful page-1-through-final-page synchronization and pending-change processing. `SqliteMediaRepository` will implement that capability transactionally using external SQL, while schema version 3 adds nullable `media.source_removed_at` and active reads filter removed rows.

**Tech Stack:** C++20, Qt 6 Core/Sql/Test, SQLite, CMake/CTest, GraphQL JSON fixtures

**Spec:** `docs/superpowers/specs/2026-09-25-anilist-authoritative-snapshot-reconciliation-design.md`

## Global Constraints

- A snapshot is authoritative only when it starts at page `1`, persists every page, naturally reaches `hasNextPage: false`, and completes pending-change processing.
- Failed, timed-out, invalid-page, and non-initial-page synchronization must not reconcile absence.
- Logical removal must preserve the media row, local-authoritative fields, pending changes, cover metadata, and cover files.
- Upsert must clear `source_removed_at` for returning media without overwriting existing local-authoritative fields.
- SQL and GraphQL remain external resources; QML performs no filtering or reconciliation.
- The removed-media UI and permanent deletion remain out of scope.

## Review Focus

- An empty but complete page-1 snapshot must remove all active media; Task 3 tests this explicitly.
- Duplicate IDs across pages must be reconciled once without changing correctness; Task 3 asserts the unique observed-ID set.
- A database already at schema version 2 with media and cover rows must migrate without data loss; Task 1 tests both rows.
- Reconciliation failure must roll back all source-removal timestamps and fail synchronization; Tasks 2 and 3 test repository rollback and propagated failure.
- A returning media with a changed cover URL must reactivate while retaining local progress and its old cache row for the cover replacement flow; Task 2 asserts those fields and relationships.

---

### Task 1: Add the version-3 media removal migration

**Files:**
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `tests/unit/SqliteDatabaseTests.cpp`

**Interfaces:**
- Consumes: existing `SqliteDatabase::migrate() -> bool` and `schema_version` table.
- Produces: fresh and upgraded databases containing nullable `media.source_removed_at` and schema versions `{1, 2, 3}`.

- [ ] **Step 1: Write failing migration tests**

Add `migrationCreatesSourceRemovalVersionThree()` and `migrationUpgradesVersionTwoWithoutDataLoss()` to `SqliteDatabaseTests`. Assert that a fresh database exposes nullable `source_removed_at`; an explicitly constructed version-2 schema with one media and one `cover_cache` row gains the column; both rows remain; and versions are exactly `{1, 2, 3}`. Update the existing schema-column and idempotency expectations.

- [ ] **Step 2: Run the focused test and verify RED**

Run: `cmake --build cmake-build-debug --target SqliteDatabaseTests && ctest --test-dir cmake-build-debug -R "^SqliteDatabaseTests$" --output-on-failure`

Expected: FAIL because version 3 and `source_removed_at` do not exist.

- [ ] **Step 3: Implement migration version 3**

Update `SqliteDatabase::migrate()` so fresh `media` creation includes `source_removed_at TEXT`, existing schemas conditionally execute `ALTER TABLE media ADD COLUMN source_removed_at TEXT` only when version 3 is absent, and version 3 is recorded in the same transaction. Preserve useful diagnostics and idempotency.

- [ ] **Step 4: Run the focused test and verify GREEN**

Run: `cmake --build cmake-build-debug --target SqliteDatabaseTests && ctest --test-dir cmake-build-debug -R "^SqliteDatabaseTests$" --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit the migration**

```bash
git add src/infrastructure/database/SqliteDatabase.cpp tests/unit/SqliteDatabaseTests.cpp
git commit -m "feat: add source removal schema migration"
```

### Task 2: Implement transactional snapshot reconciliation in SQLite

**Files:**
- Create: `src/application/media/IMediaSnapshotReconciler.h`
- Create: `resources/sqlite/queries/read-active-media-ids.sql`
- Create: `resources/sqlite/queries/mark-media-source-removed.sql`
- Modify: `src/application/media/IMediaRepository.h`
- Modify: `src/infrastructure/database/SqliteMediaRepository.h`
- Modify: `src/infrastructure/database/SqliteMediaRepository.cpp`
- Modify: `resources/sqlite/queries/upsert-media.sql`
- Modify: `resources/sqlite/queries/read-media.sql`
- Modify: `resources/sqlite/sqlite-queries.json`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.h`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.cpp`
- Modify: `tests/unit/SqliteRepositoryTests.cpp`
- Modify: `tests/unit/SqliteQueryConfigurationTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: version-3 `media.source_removed_at` from Task 1 and existing remote-authoritative media upsert behavior.
- Produces: `IMediaSnapshotReconciler::reconcileAuthoritativeSnapshot(const QSet<int> &observedMediaIds, int &removedCount, QString &error) -> bool`; `SqliteMediaRepository` implements it and `IMediaRepository` inherits it.

- [ ] **Step 1: Write failing repository and query-configuration tests**

Add tests that assert: unseen active media receive one UTC ISO timestamp; observed media remain active; already removed rows are not counted again; an empty observed set removes every active row; a forced update failure rolls back all marks; `readAll()` excludes removed media; and upserting a removed media clears the timestamp while preserving `consumed_chapters`, `personal_score`, pending changes, and `cover_cache`. Extend query-configuration tests to require both new external SQL resource paths.

- [ ] **Step 2: Run focused tests and verify RED**

Run: `cmake --build cmake-build-debug --target SqliteRepositoryTests SqliteQueryConfigurationTests && ctest --test-dir cmake-build-debug -R "^(SqliteRepositoryTests|SqliteQueryConfigurationTests)$" --output-on-failure`

Expected: FAIL because the reconciliation interface, queries, and active filtering do not exist.

- [ ] **Step 3: Add the reconciliation contract and external queries**

Create `IMediaSnapshotReconciler` with the exact signature above. Make `IMediaRepository` inherit it. Add configured SQL for reading active IDs and for `UPDATE media SET source_removed_at = :source_removed_at WHERE id = :id AND source_removed_at IS NULL`; expose both paths through `SqliteQueryConfiguration` and package them in `CMakeLists.txt`.

- [ ] **Step 4: Implement repository reconciliation and reactivation**

Extend `SqliteMediaRepository` construction with `readActiveMediaIdsQuery` and `markSourceRemovedQuery`. Reconcile in one transaction, bind one shared `QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)` timestamp, set `removedCount` only for newly marked rows, and roll back on any failure. Update external upsert SQL to set `source_removed_at = NULL` on conflict, and active read SQL to use `WHERE source_removed_at IS NULL`.

- [ ] **Step 5: Run focused tests and verify GREEN**

Run: `cmake --build cmake-build-debug --target SqliteRepositoryTests SqliteQueryConfigurationTests && ctest --test-dir cmake-build-debug -R "^(SqliteRepositoryTests|SqliteQueryConfigurationTests)$" --output-on-failure`

Expected: PASS.

- [ ] **Step 6: Commit the persistence capability**

```bash
git add src/application/media/IMediaSnapshotReconciler.h src/application/media/IMediaRepository.h src/infrastructure/database/SqliteMediaRepository.h src/infrastructure/database/SqliteMediaRepository.cpp resources/sqlite/queries/read-active-media-ids.sql resources/sqlite/queries/mark-media-source-removed.sql resources/sqlite/queries/upsert-media.sql resources/sqlite/queries/read-media.sql resources/sqlite/sqlite-queries.json src/infrastructure/database/SqliteQueryConfiguration.h src/infrastructure/database/SqliteQueryConfiguration.cpp tests/unit/SqliteRepositoryTests.cpp tests/unit/SqliteQueryConfigurationTests.cpp CMakeLists.txt
git commit -m "feat: reconcile authoritative media snapshots"
```

### Task 3: Gate reconciliation on a complete AniList synchronization

**Files:**
- Modify: `src/application/anilist/AniListSyncService.h`
- Modify: `src/application/anilist/AniListSyncService.cpp`
- Modify: `tests/unit/AniListFlowTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `IMediaSnapshotReconciler::reconcileAuthoritativeSnapshot(...)` from Task 2, `MediaPage::hasNextPage`, and existing `AniListPendingChangeProcessor` behavior.
- Produces: `AniListSyncService(IMediaDataSource &, IMediaWriter &, IMediaSnapshotReconciler *snapshotReconciler = nullptr, AniListPendingChangeProcessor *pendingProcessor = nullptr, int timeoutMs = 0)` with reconciliation only for complete page-1 snapshots.

- [ ] **Step 1: Write failing synchronization tests with a recording reconciler**

Extend `AniListFlowTests` with controlled page sources and a `RecordingSnapshotReconciler`. Assert that a successful multi-page page-1 sync calls reconciliation once after all upserts with a unique `QSet<int>`; `hasNextPage: true` requests the next page before reconciliation; an empty complete snapshot reconciles an empty set; starting at page 2 does not reconcile; and fetch, upsert, timeout, wrong-page, pending-processing, and reconciliation failures return the proper failure without an unauthorized reconciliation call.

- [ ] **Step 2: Run the focused test and verify RED**

Run: `cmake --build cmake-build-debug --target AniListFlowTests && ctest --test-dir cmake-build-debug -R "^AniListFlowTests$" --output-on-failure`

Expected: FAIL because `AniListSyncService` has no snapshot reconciler.

- [ ] **Step 3: Implement authoritative-snapshot gating**

Inject the nullable reconciler through the exact constructor signature above. Preserve unique IDs in `QSet<int>`, follow every `hasNextPage: true`, process pending changes first, then call reconciliation only when the original `filter.startingPage == 1` and the terminal page was reached successfully. Classify reconciliation errors through `AniListSyncErrorClassifier` and leave partial synchronization behavior unchanged.

- [ ] **Step 4: Run the focused test and verify GREEN**

Run: `cmake --build cmake-build-debug --target AniListFlowTests && ctest --test-dir cmake-build-debug -R "^AniListFlowTests$" --output-on-failure`

Expected: PASS.

- [ ] **Step 5: Commit the synchronization rule**

```bash
git add src/application/anilist/AniListSyncService.h src/application/anilist/AniListSyncService.cpp tests/unit/AniListFlowTests.cpp CMakeLists.txt
git commit -m "feat: reconcile complete AniList synchronizations"
```

### Task 4: Make the recorded fixture authoritative and wire startup end to end

**Files:**
- Modify: `tests/fixtures/graphql/page-response.json`
- Modify: `src/infrastructure/anilist/RecordedGraphQlAniListDataSource.cpp`
- Modify: `src/app/InitialSyncCoordinator.h`
- Modify: `src/app/InitialSyncCoordinator.cpp`
- Modify: `src/app/ApplicationComposition.cpp`
- Modify: `tests/unit/AniListGraphQlParsingTests.cpp`
- Modify: `tests/unit/InitialSyncCoordinatorTests.cpp`
- Modify: `planning.md`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: repository query paths from Task 2 and the `AniListSyncService` constructor from Task 3.
- Produces: startup synchronization that honors recorded GraphQL pagination and logically removes stale active rows after the complete 50-item fixture is persisted.

- [ ] **Step 1: Write failing fixture and startup integration tests**

Assert that `page-response.json` parses with `hasNextPage == false`; `RecordedGraphQlAniListDataSource` returns the document value without rewriting pagination; and a database preloaded with synthetic media ID `30002`, local progress, and cover metadata ends startup synchronization with 50 active fixture media, ID `30002` carrying non-null `source_removed_at`, and its local/cover rows preserved.

- [ ] **Step 2: Run focused tests and verify RED**

Run: `cmake --build cmake-build-debug --target AniListGraphQlParsingTests InitialSyncCoordinatorTests && ctest --test-dir cmake-build-debug -R "^(AniListGraphQlParsingTests|InitialSyncCoordinatorTests)$" --output-on-failure`

Expected: FAIL because the fixture still reports another page, the adapter overrides it, and startup does not supply reconciliation queries/capability.

- [ ] **Step 3: Correct fixture pagination and remove the adapter override**

Set `data.Page.pageInfo.hasNextPage` to `false` in the recorded response. Remove the code that forcibly sets `result.hasNextPage` and `result.totalPages`; preserve page-number validation.

- [ ] **Step 4: Wire reconciliation queries through startup composition**

Load the two new configured SQL files in `ApplicationComposition` and `InitialSyncCoordinator`, pass them to `SqliteMediaRepository`, and construct `AniListSyncService` with the repository as both writer and reconciler. Keep failures non-blocking for the window through the existing coordinator signaling and logging path.

- [ ] **Step 5: Update the priorities-only project plan**

Update `planning.md` concisely to mark authoritative soft deletion/reactivation as implemented and leave the removed-media view plus permanent deletion as future priorities, without implementation detail.

- [ ] **Step 6: Run focused tests and verify GREEN**

Run: `cmake --build cmake-build-debug --target AniListGraphQlParsingTests InitialSyncCoordinatorTests && ctest --test-dir cmake-build-debug -R "^(AniListGraphQlParsingTests|InitialSyncCoordinatorTests)$" --output-on-failure`

Expected: PASS.

- [ ] **Step 7: Run complete verification**

Run: `cmake --build cmake-build-debug && ctest --test-dir cmake-build-debug --output-on-failure && git diff --check`

Expected: build succeeds, every CTest test passes, and `git diff --check` emits no errors.

- [ ] **Step 8: Commit the end-to-end integration**

```bash
git add tests/fixtures/graphql/page-response.json src/infrastructure/anilist/RecordedGraphQlAniListDataSource.cpp src/app/InitialSyncCoordinator.h src/app/InitialSyncCoordinator.cpp src/app/ApplicationComposition.cpp tests/unit/AniListGraphQlParsingTests.cpp tests/unit/InitialSyncCoordinatorTests.cpp planning.md CMakeLists.txt
git commit -m "feat: reconcile recorded AniList library at startup"
```
