# User Preferences Backend Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Persist score, cover-quality, and synchronization preferences in SQLite and apply a successful settings-screen save to the running application immediately.

**Architecture:** A typed `UserPreferences` application model is read and atomically replaced through an application repository port backed by SQLite. A screen-specific controller owns the editable draft; after a successful commit it emits the complete model to focused runtime setters on the home and synchronization controllers. Packaged JSON supplies first-run defaults, while persisted database values take precedence.

**Tech Stack:** C++20, Qt 6 Core/Sql/Qml/Quick, QML, SQLite, CMake, Qt Test/CTest

**Spec:** `docs/superpowers/specs/2026-09-26-user-preferences-backend-design.md`

## Global Constraints

- Keep `main.cpp` limited to Qt/QML initialization and signal wiring; composition and persistence construction stay in `src/app/`.
- Use English identifiers, JSON keys, SQL names, and internal API names; user-facing QML copy remains Portuguese.
- Keep SQL text in resource files registered through `resources/sqlite/sqlite-queries.json`; do not embed repository SQL in C++.
- Preserve existing user databases through a transactional schema migration; never use the real user database in tests.
- `Settings.json` remains the source of operational settings and gains only typed first-run user-preference defaults.
- QML receives allowed keys, ranges, and labels from `SettingsController`; it does not define backend policy.
- Synchronization intervals are bounded to 300000-86400000 ms; the initial selectable values are 900000, 1800000, 3600000, 10800000, 21600000, 43200000, and 86400000 ms.
- A failed save changes neither persisted preferences nor runtime consumers.
- Settings and infrastructure failures must not prevent the application window from opening.

## Review Focus

- A database containing schema versions 1-4 and existing media/cache rows must upgrade without data loss; Task 2 adds an upgrade test with sentinel rows.
- NaN, infinity, non-divisible score steps, unsupported cover keys, and out-of-range intervals must be rejected before SQL executes; Tasks 1 and 5 pin these inputs.
- Disabling synchronization during an active operation must not cancel it or restart the scheduler after completion; Task 4 covers this race.
- A cover-quality change must keep the current displayed file when a replacement fails; Tasks 3 and 6 cover resolver and controller/coordinator behavior.
- A repository failure after the user edits multiple fields must preserve the complete draft, report the error, and emit no apply signal; Task 5 tests the atomic UI contract.

---

### Task 1: Typed preferences and packaged defaults

**Files:**
- Create: `src/application/configuration/UserPreferences.h`
- Create: `src/application/configuration/UserPreferencesValidator.h`
- Create: `src/application/configuration/UserPreferencesValidator.cpp`
- Modify: `src/application/configuration/Settings.h`
- Modify: `src/infrastructure/configuration/JsonSettingsReader.cpp`
- Modify: `Settings.json`
- Modify: `tests/unit/JsonSettingsReaderTests.cpp`
- Create: `tests/unit/UserPreferencesValidatorTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `struct UserPreferences { double scoreMinimum; double scoreMaximum; double scoreStep; CoverQuality coverQuality; bool synchronizationEnabled; int synchronizationIntervalMs; };`
- Produces: `UserPreferencesValidationResult ValidateUserPreferences(const UserPreferences &preferences)` where the result exposes `bool valid` and `QString error`.
- Produces: `Settings::userPreferences` populated by `JsonSettingsReader::read`.

- [ ] **Step 1: Write failing JSON-reader tests** for the approved defaults `0`, `10`, `1`, `medium`, `true`, and `3600000`, plus wrong boolean/string/numeric types and missing `userPreferences` falling back to the same safe defaults.
- [ ] **Step 2: Run `cmake --build cmake-build-validation --target JsonSettingsReaderTests && ctest --test-dir cmake-build-validation -R JsonSettingsReaderTests --output-on-failure`** and verify the new assertions fail.
- [ ] **Step 3: Add `UserPreferences` to `Settings`, add the exact `userPreferences` object to `Settings.json`, and extend `JsonSettingsReader::read` with strict JSON type checks and controlled fallback defaults.**
- [ ] **Step 4: Re-run `JsonSettingsReaderTests`** and expect all cases to pass.
- [ ] **Step 5: Write failing validator tests** for a valid 0-10/1 scale, a valid 0-100/5 scale, non-finite values, minimum equal to or above maximum, zero/negative/oversized/non-divisible steps, unsupported `CoverQuality` casts, and non-positive or controller-out-of-range intervals.
- [ ] **Step 6: Implement `ValidateUserPreferences(const UserPreferences &) -> UserPreferencesValidationResult`** with named synchronization bounds of 300000 and 86400000 ms shared with the later controller.
- [ ] **Step 7: Build and run `UserPreferencesValidatorTests` and `JsonSettingsReaderTests`** and expect both executables to pass.
- [ ] **Step 8: Commit the task** with `git add Settings.json CMakeLists.txt src/application/configuration src/infrastructure/configuration/JsonSettingsReader.cpp tests/unit/JsonSettingsReaderTests.cpp tests/unit/UserPreferencesValidatorTests.cpp && git commit -m "feat: define user preference defaults"`.

### Task 2: SQLite preference repository and schema migration

**Files:**
- Create: `src/application/configuration/IUserPreferencesRepository.h`
- Create: `src/infrastructure/database/SqliteUserPreferencesRepository.h`
- Create: `src/infrastructure/database/SqliteUserPreferencesRepository.cpp`
- Create: `resources/sqlite/queries/read-user-preferences.sql`
- Create: `resources/sqlite/queries/upsert-user-preferences.sql`
- Modify: `resources/sqlite/sqlite-queries.json`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.h`
- Modify: `src/infrastructure/database/SqliteQueryConfiguration.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `tests/unit/SqliteDatabaseTests.cpp`
- Create: `tests/unit/SqliteUserPreferencesRepositoryTests.cpp`
- Modify: `tests/unit/SqliteQueryConfigurationTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `UserPreferences` and `ValidateUserPreferences` from Task 1.
- Produces: `IUserPreferencesRepository::read(UserPreferences &preferences, bool &found, QString &error) -> bool`.
- Produces: `IUserPreferencesRepository::replace(const UserPreferences &preferences, QString &error) -> bool`.
- Produces: query configuration members `readUserPreferencesPath` and `upsertUserPreferencesPath`.

- [ ] **Step 1: Add failing migration tests** asserting version 5 creates the singleton `user_preferences` table, records the version once, rejects a second row through `CHECK (id = 1)`, and upgrades a version-4 database while preserving sentinel media and cover-cache rows.
- [ ] **Step 2: Run `SqliteDatabaseTests`** and confirm the version-5 assertions fail.
- [ ] **Step 3: Extend `SqliteDatabase::migrate()`** to create the exact typed table from the spec inside the existing transaction and require schema versions 1 through 5 before commit.
- [ ] **Step 4: Re-run `SqliteDatabaseTests`** and expect all migration cases to pass.
- [ ] **Step 5: Add failing query-configuration tests** requiring the two new resource paths and rejecting an incomplete configuration.
- [ ] **Step 6: Add both external SQL files, aliases, JSON entries, CMake resources, and `SqliteQueryConfiguration` members/load validation.**
- [ ] **Step 7: Run `SqliteQueryConfigurationTests`** and expect all cases to pass.
- [ ] **Step 8: Write failing repository tests** for not-found reads, exact type round-trip, replacing the singleton row, validator rejection before execution, and transaction rollback when the upsert SQL fails.
- [ ] **Step 9: Implement `SqliteUserPreferencesRepository`** using the injected opened connection and external read/upsert statements; `replace` validates then begins, executes, and commits one transaction.
- [ ] **Step 10: Build and run `SqliteUserPreferencesRepositoryTests`, `SqliteDatabaseTests`, and `SqliteQueryConfigurationTests`** and expect all to pass.
- [ ] **Step 11: Commit the task** with only repository, SQL resource, migration, tests, and CMake changes using message `feat: persist user preferences in sqlite`.

### Task 3: Persist and resolve cover variants

**Files:**
- Modify: `src/domain/media/Media.h`
- Modify: `src/infrastructure/anilist/AniListMediaMapper.cpp`
- Create: `src/application/covers/CoverSourceResolver.h`
- Create: `src/application/covers/CoverSourceResolver.cpp`
- Modify: `resources/sqlite/queries/upsert-media.sql`
- Modify: `resources/sqlite/queries/read-media.sql`
- Modify: `src/infrastructure/database/SqliteMediaMapper.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `tests/unit/AniListGraphQlParsingTests.cpp`
- Modify: `tests/unit/SqliteDatabaseTests.cpp`
- Modify: `tests/unit/SqliteRepositoryTests.cpp`
- Create: `tests/unit/CoverSourceResolverTests.cpp`
- Modify: `tests/fixtures/media-library.json`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: media members `CoverMediumUrl`, `CoverLargeUrl`, and `CoverExtraLargeUrl`, while retaining `CoverUrl` as the original/effective backward-compatible URL.
- Produces: `QString ResolveCoverSource(const Media &media, CoverQuality requestedQuality)`.
- Consumes later: Home cover requests use `ResolveCoverSource` rather than `Media::CoverUrl` directly.

- [ ] **Step 1: Add failing mapper and repository tests** proving all three AniList URLs survive DTO mapping, SQLite upsert, and read round-trip while legacy rows with only `cover_url` remain readable.
- [ ] **Step 2: Add failing migration tests** proving version 6 adds nullable cover-variant columns to existing `media` tables without changing sentinel values and is recorded exactly once after the columns exist.
- [ ] **Step 3: Run `AniListGraphQlParsingTests`, `SqliteDatabaseTests`, and `SqliteRepositoryTests`** and confirm the new cases fail.
- [ ] **Step 4: Extend `Media`, AniList mapping, migration, SQL, database mapping, and fixtures** to preserve the three variants; keep remote variant fields remote-authoritative during upsert and require schema versions 1 through 6 before migration commit.
- [ ] **Step 5: Re-run the three affected suites** and expect the round-trip and upgrade cases to pass.
- [ ] **Step 6: Write failing `CoverSourceResolverTests`** for requested URL, stored original fallback, exact alternative orders (`medium -> large -> extraLarge`, `large -> medium -> extraLarge`, `extraLarge -> large -> medium`), and no URL.
- [ ] **Step 7: Implement `ResolveCoverSource(const Media &, CoverQuality) -> QString`** as a deterministic side-effect-free application helper.
- [ ] **Step 8: Build and run `CoverSourceResolverTests` plus the three regression suites** and expect all to pass.
- [ ] **Step 9: Commit the task** with message `feat: preserve and resolve cover variants`.

### Task 4: Runtime synchronization scheduling

**Files:**
- Modify: `src/app/InitialSyncCoordinator.h`
- Modify: `src/app/InitialSyncCoordinator.cpp`
- Modify: `tests/unit/InitialSyncCoordinatorTests.cpp`

**Interfaces:**
- Produces: `void configureAutomaticSynchronization(bool enabled, int intervalMs)`.
- Produces: `bool automaticSynchronizationEnabled() const` and `int synchronizationIntervalMs() const` for deterministic tests.
- Contract: configuration never cancels an active worker; completion starts the timer only when enabled.

- [ ] **Step 1: Write failing coordinator tests** for disabled startup, disabling while idle, disabling during an active operation, enabling while idle, enabling while active, and restarting an active timer from zero when the interval changes.
- [ ] **Step 2: Run `InitialSyncCoordinatorTests`** and verify the new scheduling tests fail.
- [ ] **Step 3: Implement `configureAutomaticSynchronization(bool, int)`** by storing enabled state, updating `QTimer::interval`, stopping when disabled, and starting/restarting only when enabled and no operation is active; guard completion with the stored enabled state.
- [ ] **Step 4: Re-run `InitialSyncCoordinatorTests`** and expect the complete executable to pass without timing flakes.
- [ ] **Step 5: Commit the task** with `git add src/app/InitialSyncCoordinator.* tests/unit/InitialSyncCoordinatorTests.cpp && git commit -m "feat: reconfigure automatic synchronization"`.

### Task 5: Settings screen controller and atomic draft workflow

**Files:**
- Create: `src/presentation/settings/SettingsController.h`
- Create: `src/presentation/settings/SettingsController.cpp`
- Create: `tests/unit/SettingsControllerTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `IUserPreferencesRepository`, `UserPreferences`, validator bounds, and `CoverQualityName` from Tasks 1-2.
- Produces QML properties: persisted/draft score values, `coverQualityKey`, `synchronizationEnabled`, `synchronizationIntervalMs`, option lists, `dirty`, `valid`, `saving`, `statusMessage`, and `errorMessage`.
- Produces invokables: `SetScoreScale(double minimum, double maximum, double step)`, `SetCoverQuality(QString key)`, `SetSynchronizationEnabled(bool enabled)`, `SetSynchronizationInterval(int intervalMs)`, `Save()`, and `Discard()`.
- Produces signal: `preferencesApplied(UserPreferences preferences)` emitted only after a successful commit.

- [ ] **Step 1: Create a fake repository and failing controller tests** for initial values/options, every draft setter, dirty state, section-independent draft preservation, discard, successful save, and the exact emitted complete model.
- [ ] **Step 2: Add failing tests** proving NaN/infinity/non-divisible scales, unsupported quality keys, and invalid intervals never call `replace`.
- [ ] **Step 3: Add a failing repository-error test** asserting all edited draft values remain, `errorMessage` is populated, persisted values remain unchanged, and `preferencesApplied` is not emitted.
- [ ] **Step 4: Build and run `SettingsControllerTests`** and confirm the test target fails before the controller exists.
- [ ] **Step 5: Implement the controller with one persisted snapshot and one draft**; derive dirty/valid state after each setter and expose interval choices for 15 minutes, 30 minutes, 1 hour, 3 hours, 6 hours, 12 hours, and 24 hours in backend-provided `QVariantList` models.
- [ ] **Step 6: Implement `Save()` and `Discard()`** according to the atomic contract, including reentrancy protection through `saving`.
- [ ] **Step 7: Re-run `SettingsControllerTests`** and expect all cases to pass.
- [ ] **Step 8: Commit the task** with message `feat: add settings presentation controller`.

### Task 6: Composition and immediate runtime application

**Files:**
- Modify: `src/app/ApplicationComposition.h`
- Modify: `src/app/ApplicationComposition.cpp`
- Modify: `src/presentation/home/HomeScreenController.h`
- Modify: `src/presentation/home/HomeScreenController.cpp`
- Modify: `tests/unit/HomeScreenControllerTests.cpp`
- Modify: `main.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: repository/query contracts, controller, resolver, and scheduler methods from Tasks 2-5.
- Produces: `ApplicationContext::userPreferencesRepository` and `ApplicationContext::userPreferences` loaded from SQLite or seeded from packaged defaults.
- Produces: `HomeScreenController::ConfigureCoverQuality(CoverQuality quality)`; it changes future request resolution without clearing current cover sources.
- Publishes: QML context property `settingsController`.

- [ ] **Step 1: Add failing home-controller tests** proving a quality change preserves current `coverSource`, resolves future cover-window requests with the new preferred/fallback URL, and leaves the old source visible when the replacement reports failure.
- [ ] **Step 2: Run `HomeScreenControllerTests`** and verify these cases fail.
- [ ] **Step 3: Replace direct `Media::CoverUrl` request construction with `ResolveCoverSource` and implement `ConfigureCoverQuality`** without calling `ClearCovers`.
- [ ] **Step 4: Re-run `HomeScreenControllerTests`** and expect all cases to pass.
- [ ] **Step 5: Extend application composition** to construct `SqliteUserPreferencesRepository`, read or seed from `Settings::userPreferences`, retain safe defaults and a controlled diagnostic on failure, and expose the repository/current model through `ApplicationContext`.
- [ ] **Step 6: Construct `SettingsController` in `main.cpp`, publish it to QML, apply initial preferences before initial synchronization, and connect `preferencesApplied` to score scale, cover quality, and `InitialSyncCoordinator::configureAutomaticSynchronization`.**
- [ ] **Step 7: Gate the startup call to `InitialSyncCoordinator::start()` on `synchronizationEnabled`; preserve the existing shutdown order.**
- [ ] **Step 8: Build `HaikenAnime`, `HomeScreenControllerTests`, `InitialSyncCoordinatorTests`, and `SettingsControllerTests`; run those tests** and expect all to pass.
- [ ] **Step 9: Commit the task** with message `feat: apply preferences at runtime`.

### Task 7: Functional QML settings controls

**Files:**
- Modify: `resources/qml/SettingsScreen.qml`
- Modify: `resources/qml/Main.qml`
- Modify: `resources/qml/Home.qml`

**Interfaces:**
- Consumes: the `settingsController` context property and Task 5 properties/invokables.
- Produces: editable score, cover-quality, sync-enabled, and interval controls; save feedback; dirty-navigation confirmation; unavailable states for deferred categories.

- [ ] **Step 1: Bind Account and synchronization controls** to backend option models and draft setters; use spin boxes or selectors whose minimum, maximum, and step come from the controller.
- [ ] **Step 2: Bind the Appearance cover-quality selector** to controller options and `SetCoverQuality`, showing persisted/draft selection accurately.
- [ ] **Step 3: Bind Save Changes** to `Save()`, enabling it only for `dirty && valid && !saving`, and render saving, success, validation, and repository-error messages accessibly.
- [ ] **Step 4: Add the unsaved-changes confirmation** so Back requests `Discard()` only after explicit confirmation; clean navigation returns immediately.
- [ ] **Step 5: Mark nonfunctional Account authorization, library, filters, downloads, and history controls unavailable** without removing the approved overview or lateral navigation.
- [ ] **Step 6: When Home becomes visible after a cover preference change, request the current preview/full cover windows again** while retaining existing displayed sources until replacements arrive.
- [ ] **Step 7: Run `qmllint` on `SettingsScreen.qml`, `Main.qml`, and `Home.qml`**; expect no syntax or missing-property errors, while documenting pre-existing unqualified-access warnings separately.
- [ ] **Step 8: Build `HaikenAnime` and run a short offscreen startup smoke test**; expect successful QML module loading and no runtime binding errors.
- [ ] **Step 9: Inspect the settings UI at 1440x900 and the 1100x700 minimum**; verify navigation, scrolling, focus, save state, validation, confirmation, and readable disabled sections.
- [ ] **Step 10: Commit the task** with `git add resources/qml/SettingsScreen.qml resources/qml/Main.qml resources/qml/Home.qml && git commit -m "feat: connect settings screen to preferences"`.

### Task 8: Full regression and delivery verification

**Files:**
- Modify only files required to correct failures introduced by Tasks 1-7.

**Interfaces:**
- Consumes: all previous task deliverables.
- Produces: a verified settings flow with documented validation evidence.

- [ ] **Step 1: Configure and build the validation tree** with `cmake -S . -B cmake-build-validation -DBUILD_TESTING=ON` followed by `cmake --build cmake-build-validation -j 4`; expect a successful build.
- [ ] **Step 2: Run `ctest --test-dir cmake-build-validation --output-on-failure`** and require the complete suite to pass.
- [ ] **Step 3: Run `git diff --check`** and require no whitespace errors; ignore only explicit line-ending notices.
- [ ] **Step 4: Run a final offscreen startup smoke test** and require no QML load, SQLite migration, or settings-seeding errors.
- [ ] **Step 5: Perform a final manual flow**: edit all supported fields, navigate between sections, save once, confirm immediate score/cover/scheduler behavior, restart, and confirm persisted values reload.
- [ ] **Step 6: Review `git status --short` and the scoped diff** to ensure unrelated pre-existing worktree changes were neither overwritten nor committed.
- [ ] **Step 7: Commit only any verification-driven corrections** with message `fix: complete user preferences integration`; skip this commit if no correction was necessary.
