# Frontend, Observability, and Catalog Improvements Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver the approved frontend clarity, startup hygiene, scanner observability, persistent presentation preferences, richer media details, localization, and explicit year-and-season AniList catalog as separately validated increments.

**Architecture:** Extend the existing typed preferences, SQLite migrations, screen-specific controllers, external SQL/GraphQL resources, and presentation-only QML boundaries. New catalog behavior receives a dedicated application/data-source contract and screen; home remains focused on the user's library. Each task is a review gate and implementation stops after its verification until the user accepts it.

**Tech Stack:** C++20, Qt 6.8.3, Qt Quick/QML, Qt Network, Qt Linguist, SQLite, CMake/Ninja, QtTest/CTest.

**Spec:** `docs/superpowers/specs/2026-09-27-frontend-observability-catalog-design.md`

## Global Constraints

- Implement tasks strictly in order and stop after each task for review and user acceptance.
- QML receives presentation-ready data; it does not define persistence defaults, sorting policy, GraphQL filters, title fallback, or translation keys.
- Options shown by selectors come from controllers/backend contracts, not hard-coded QML arrays.
- Optional-feature failures remain non-fatal to main-window startup.
- Use external SQL and GraphQL resources.
- Preserve existing media, pending changes, cover cache, and local-file inventory through every migration.
- Do not add higher-quality cover downloads for image preview.
- Run focused tests, QML compilation, full build, and complete CTest before claiming a task complete.

## Review Focus

- Narrow settings widths: extension choices must reflow without clipping, overlap, lost focus, or changed selection.
- Corrupt or obsolete persisted option keys: startup must use deterministic fallback rather than blank controls or invalid state.
- Startup filesystem failures: temporary cleanup and logging failures must remain bounded to application-owned paths and non-fatal.
- Stale asynchronous catalog responses: a previous year/season response must never replace the active selection.
- Missing AniList fields and hostile URLs/HTML: details remain usable, malformed links are omitted, and plain text remains selectable.

---

### Task 1: Responsive scan-extension grid

**Files:**
- Modify: `resources/qml/SettingsScreen.qml`
- Create: `tests/unit/QmlStructureTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `SettingsController::availableScanExtensions` and `selectedScanExtensions`.
- Produces: responsive row-major extension layout with unchanged toggle behavior.

- [ ] **Step 1: Add a structural regression check** that fails unless the extension container derives its column count from its own available width and the delegate has a bounded cell width.
- [ ] **Step 2: Run the QML check and record the expected failure** against the current single-column rendering behavior.
- [ ] **Step 3: Replace the fragile `GridLayout` sizing** with a responsive grid/flow whose breakpoints yield four, two, and one columns without defining extension values in QML.
- [ ] **Step 4: Build QML and run `SettingsControllerTests`**, expecting compilation and all focused tests to pass.
- [ ] **Step 5: Validate visually** at wide, intermediate, and narrow settings-panel widths, including keyboard focus and persisted checks.
- [ ] **Step 6: Run full CTest and commit** with `fix: make scan extensions responsive`.
- [ ] **Review gate:** stop and obtain acceptance before Task 2.

### Task 2: Startup cleanup of abandoned cover temporaries

**Files:**
- Create: `src/infrastructure/covers/CoverTemporaryStore.h`
- Create: `src/infrastructure/covers/CoverTemporaryStore.cpp`
- Create: `tests/unit/CoverTemporaryStoreTests.cpp`
- Modify: `src/infrastructure/covers/QtCoverDownloader.h`
- Modify: `src/infrastructure/covers/QtCoverDownloader.cpp`
- Modify: `src/app/ApplicationComposition.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `CoverTemporaryStore(QString rootPath)` and `bool ClearAbandoned(int &removedFiles, QString &error)`.
- Consumes: the same application-owned temporary root passed to `QtCoverDownloader`.

- [ ] **Step 1: Add failing tests** for missing directory, owned files, nested unexpected directories, symlink/junction escape attempts, and one unremovable file.
- [ ] **Step 2: Run `CoverTemporaryStoreTests`** and verify RED before production code.
- [ ] **Step 3: Implement bounded cleanup** that removes files only inside the owned root, never follows links, reports removed count, and returns a diagnostic for partial failure.
- [ ] **Step 4: Compose cleanup before downloader construction**, logging failure without setting `initializationError`.
- [ ] **Step 5: Run cover and composition focused tests**, then the full build and CTest.
- [ ] **Step 6: Perform a controlled startup smoke** with abandoned temporary fixtures and confirm the persistent cache is untouched.
- [ ] **Step 7: Commit** with `feat: clean abandoned cover downloads`.
- [ ] **Review gate:** stop and obtain acceptance before Task 3.

### Task 3: Local-library scan lifecycle logging

**Files:**
- Modify: `src/infrastructure/logging/AsyncLogger.h`
- Modify: `src/infrastructure/logging/AsyncLogger.cpp`
- Modify: `src/app/LocalLibraryScanCoordinator.h`
- Modify: `src/app/LocalLibraryScanCoordinator.cpp`
- Modify: `src/app/ApplicationComposition.cpp`
- Modify: `tests/unit/LocalLibraryScanCoordinatorTests.cpp`
- Modify: `tests/unit/LocalLibraryScanCompositionTests.cpp`

**Interfaces:**
- Adds: `LogCategory::LocalLibrary`.
- Adds: `LocalLibraryScanCoordinator::setLogger(AsyncLogger *logger)` without transferring ownership.

- [ ] **Step 1: Add failing logger/coordinator tests** for start, successful totals, zero-result warning, enumeration/repository error, and interrupted shutdown.
- [ ] **Step 2: Run coordinator/composition tests** and capture RED.
- [ ] **Step 3: Add the category and lifecycle messages**, logging one bounded start and one terminal entry with elapsed time and scan counters.
- [ ] **Step 4: Wire the application logger during composition** while preserving worker and logger shutdown order.
- [ ] **Step 5: Run focused scanner, coordinator, composition, and logger tests**; assert no per-file success logs.
- [ ] **Step 6: Run a controlled zero-result and failure smoke**, inspect the log, then run full CTest.
- [ ] **Step 7: Commit** with `feat: log local library scans`.
- [ ] **Review gate:** stop and obtain acceptance before Task 4.

### Task 4: Persisted ordering and deterministic selector initialization

**Files:**
- Modify: `src/application/configuration/UserPreferences.h`
- Modify: `src/application/configuration/UserPreferencesValidator.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `src/infrastructure/database/SqliteUserPreferencesRepository.cpp`
- Modify: `resources/sqlite/queries/read-user-preferences.sql`
- Modify: `resources/sqlite/queries/upsert-user-preferences.sql`
- Modify: `src/presentation/home/HomeScreenController.h`
- Modify: `src/presentation/home/HomeScreenController.cpp`
- Modify: `src/presentation/settings/SettingsController.h`
- Modify: `src/presentation/settings/SettingsController.cpp`
- Modify: `resources/qml/BrowseControls.qml`
- Modify: `main.cpp`
- Modify: relevant preference/database/home/settings tests

**Interfaces:**
- Adds: `UserPreferences::homeSortKey`, default `title_asc`.
- Adds: `HomeScreenController::ConfigureInitialSort(QString key)` and signal `sortPreferenceChanged(QString key)`.
- Adds: `bool persistHomeSortPreference(ApplicationContext &, const QString &key, QString &error)` and `SettingsController::ApplyExternalHomeSortKey(QString key)` so home changes update the repository, application snapshot, and settings snapshot atomically.
- Preserves: backend-provided `availableSortOptions` and `activeSort` as the QML source of truth.

- [ ] **Step 1: Add failing migration and repository round-trip tests** for `homeSortKey`, including legacy rows and invalid stored keys.
- [ ] **Step 2: Add failing home/QML tests** proving restored sort applies before first model publication and List/Sort labels are never blank after option configuration.
- [ ] **Step 3: Implement the preference field, migration, external SQL bindings, validation, and fallback** to the first provided sort option.
- [ ] **Step 4: Persist only accepted sort changes** and keep search, media type, and list filter session-only.
- [ ] **Step 5: Fix combo initialization bindings** without hard-coded QML indices.
- [ ] **Step 6: Run preference, database, home, settings, and QML tests**, then restart the application against an isolated database to verify restoration.
- [ ] **Step 7: Run full CTest and commit** with `feat: persist home ordering`.
- [ ] **Review gate:** stop and obtain acceptance before Task 5.

### Task 5: Self-describing media cards and configurable status source

**Files:**
- Create: `src/application/configuration/CardStatusPresentation.h`
- Modify: `src/application/configuration/UserPreferences.h`
- Modify: `src/application/configuration/UserPreferencesValidator.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `src/infrastructure/database/SqliteUserPreferencesRepository.cpp`
- Modify: `resources/sqlite/queries/read-user-preferences.sql`
- Modify: `resources/sqlite/queries/upsert-user-preferences.sql`
- Modify: `src/presentation/home/HomeScreenController.h`
- Modify: `src/presentation/home/HomeScreenController.cpp`
- Modify: `src/presentation/settings/SettingsController.h`
- Modify: `src/presentation/settings/SettingsController.cpp`
- Modify: `resources/qml/MediaCard.qml`
- Modify: `resources/qml/SettingsScreen.qml`
- Modify: home/settings/preferences/database tests

**Interfaces:**
- Adds: preference key `cardStatusPresentation` with `personal-list-status` default and `media-release-status` alternative.
- Produces card strings: `Minha lista: …`, `Exibição: …`, `Progresso …`, and `Nota …/<configured maximum>`.

- [ ] **Step 1: Add failing preference/migration tests** for both keys, invalid fallback, and restart persistence.
- [ ] **Step 2: Add failing home-model tests** for personal status default, media status alternative, unknown progress, absent score, and non-10-point scales.
- [ ] **Step 3: Implement backend-provided settings options and controller formatting**; keep QML free of semantic decisions.
- [ ] **Step 4: Adjust `MediaCard.qml` spacing** so labeled values remain readable without adding an uncontrolled extra row.
- [ ] **Step 5: Run focused tests and visually validate representative cards** at narrow and wide widths.
- [ ] **Step 6: Run full CTest and commit** with `feat: clarify media card metadata`.
- [ ] **Review gate:** stop and obtain acceptance before Task 6.

### Task 6: Selectable read-only details text

**Files:**
- Modify: `resources/qml/Home.qml`
- Modify: `tests/unit/QmlStructureTests.cpp`

**Interfaces:**
- Consumes existing selected-media presentation properties.
- Produces selectable, read-only title/synopsis/metadata without changing selection behavior.

- [ ] **Step 1: Add a QML regression check** for read-only selectable controls and preserved wrapping/elision boundaries.
- [ ] **Step 2: Replace the compact details title, synopsis, and metadata value labels** with read-only text controls supporting mouse selection and `Ctrl+C`; leave action labels and buttons unchanged.
- [ ] **Step 3: Verify clicks and selections do not activate underlying list items or close panels**.
- [ ] **Step 4: Run QML compilation, home tests, visual interaction checks, and full CTest**.
- [ ] **Step 5: Commit** with `feat: make media details selectable`.
- [ ] **Review gate:** stop and obtain acceptance before Task 7.

### Task 7: Expanded cover preview

**Files:**
- Create: `resources/qml/CoverPreview.qml`
- Modify: `resources/qml/Home.qml`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `selectedCoverSource` only.
- Produces: modal preview open/close state local to the screen; no download request interface.

- [ ] **Step 1: Add a QML structural test** for cover-source reuse and absence of cover-download/controller calls.
- [ ] **Step 2: Implement the modal preview** with aspect preservation, bounded viewport, close button, backdrop close, `Escape`, focus trap, and focus restoration.
- [ ] **Step 3: Validate placeholder, portrait, landscape, missing-file, wide, and narrow cases**.
- [ ] **Step 4: Run QML compilation, home tests, and full CTest**.
- [ ] **Step 5: Commit** with `feat: add expanded cover preview`.
- [ ] **Review gate:** stop and obtain acceptance before Task 8.

### Task 8: Restart-applied localization

**Files:**
- Create: `translations/HaikenAnime_pt_BR.ts`
- Create: `translations/HaikenAnime_en.ts`
- Create: `src/application/configuration/LanguagePreference.h`
- Create: `src/app/TranslationLoader.h`
- Create: `src/app/TranslationLoader.cpp`
- Create: `tests/unit/TranslationLoaderTests.cpp`
- Modify: `src/application/configuration/UserPreferences.h`
- Modify: `src/application/configuration/UserPreferencesValidator.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `src/infrastructure/database/SqliteUserPreferencesRepository.cpp`
- Modify: `resources/sqlite/queries/read-user-preferences.sql`
- Modify: `resources/sqlite/queries/upsert-user-preferences.sql`
- Modify: `src/presentation/settings/SettingsController.h`
- Modify: `src/presentation/settings/SettingsController.cpp`
- Modify: `resources/qml/SettingsScreen.qml`
- Modify: `CMakeLists.txt`
- Modify: `main.cpp`
- Modify: `resources/qml/BrowseControls.qml`
- Modify: `resources/qml/EditMediaPanel.qml`
- Modify: `resources/qml/Home.qml`
- Modify: `resources/qml/Main.qml`
- Modify: `resources/qml/MediaCard.qml`
- Modify: `resources/qml/StatePanel.qml`
- Modify: user-facing strings in `src/presentation/home/HomeScreenController.cpp` and `src/presentation/settings/SettingsController.cpp`

**Interfaces:**
- Adds preference `languageKey` with supported keys `pt-BR` and `en`.
- Adds `TranslationLoader::Install(QCoreApplication &, QString languageKey, QString &error)`.
- Settings exposes backend-provided language options and a restart-required message.

- [ ] **Step 1: Add failing preference/migration and translation-loader tests** for both languages, invalid key fallback, missing catalog, and install-before-QML behavior.
- [ ] **Step 2: Configure Qt Linguist in CMake** and create source catalogs covering current user-facing strings.
- [ ] **Step 3: Persist language selection** and install the translator before QML engine/component creation on the next startup only.
- [ ] **Step 4: Add the Appearance selector and restart notice**, with no attempt at runtime retranslation.
- [ ] **Step 5: Run extraction/translation build checks, focused tests, and startup smokes in both languages**.
- [ ] **Step 6: Run full CTest and commit** with `feat: add restart-applied localization`.
- [ ] **Review gate:** stop and obtain acceptance before Task 9.

### Task 9: Preferred title selection

**Files:**
- Create: `src/application/media/MediaTitleResolver.h`
- Create: `src/application/media/MediaTitleResolver.cpp`
- Create: `tests/unit/MediaTitleResolverTests.cpp`
- Modify: `src/application/configuration/UserPreferences.h`
- Modify: `src/application/configuration/UserPreferencesValidator.cpp`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `src/infrastructure/database/SqliteUserPreferencesRepository.cpp`
- Modify: `resources/sqlite/queries/read-user-preferences.sql`
- Modify: `resources/sqlite/queries/upsert-user-preferences.sql`
- Modify: `src/presentation/home/HomeScreenController.h`
- Modify: `src/presentation/home/HomeScreenController.cpp`
- Modify: `src/presentation/settings/SettingsController.h`
- Modify: `src/presentation/settings/SettingsController.cpp`
- Modify: `resources/qml/SettingsScreen.qml`
- Modify: `main.cpp`
- Modify: `tests/unit/HomeScreenControllerTests.cpp`
- Modify: `tests/unit/SettingsControllerTests.cpp`
- Modify: `tests/unit/SqliteDatabaseTests.cpp`
- Modify: `tests/unit/SqliteUserPreferencesRepositoryTests.cpp`

**Interfaces:**
- Adds preference `preferredTitleKey`: `romaji`, `english`, or `native`.
- Adds `QString ResolveMediaTitle(const Media &, QString preferredTitleKey)` using the spec fallback order.

- [ ] **Step 1: Add failing resolver tests** for every preference and every missing-field fallback combination.
- [ ] **Step 2: Add failing persistence/controller tests** proving cards, selection, search, and title sorting use the same resolved value after restart.
- [ ] **Step 3: Implement preference storage, backend options, resolver, and controller integration** without overwriting stored source titles.
- [ ] **Step 4: Add the Appearance selector** and validate home, full list, search, and sorting visually.
- [ ] **Step 5: Run focused tests and full CTest; commit** with `feat: add preferred media titles`.
- [ ] **Review gate:** stop and obtain acceptance before Task 10.

### Task 10: Extended media metadata contract

**Files:**
- Create: `src/domain/media/MediaLink.h`
- Modify: `src/domain/media/Media.h`
- Modify: `src/infrastructure/anilist/AniListMediaDto.h`
- Modify: `src/infrastructure/anilist/AniListGraphQlPageParser.h`
- Modify: `src/infrastructure/anilist/AniListGraphQlPageParser.cpp`
- Modify: `src/infrastructure/anilist/AniListMediaMapper.h`
- Modify: `src/infrastructure/anilist/AniListMediaMapper.cpp`
- Modify: `resources/anilist/queries/media-page.graphql`
- Modify: `src/infrastructure/database/SqliteDatabase.cpp`
- Modify: `src/infrastructure/database/SqliteMediaMapper.h`
- Modify: `src/infrastructure/database/SqliteMediaMapper.cpp`
- Modify: `src/infrastructure/database/SqliteMediaRepository.cpp`
- Modify: `resources/sqlite/queries/read-media.sql`
- Modify: `resources/sqlite/queries/upsert-media.sql`
- Modify: `tests/fixtures/graphql/page-response.json`
- Modify: `tests/unit/AniListGraphQlParsingTests.cpp`
- Modify: `tests/unit/SqliteDatabaseTests.cpp`
- Modify: `tests/unit/SqliteRepositoryTests.cpp`

**Interfaces:**
- Adds domain fields for season, season year, next-airing episode/timestamp, AniList URL, and structured external links.
- Adds normalized plain-text synopsis and validated HTTP(S) link output from mapping.

- [ ] **Step 1: Add failing GraphQL parsing/mapping tests** for complete data, HTML synopsis, duplicate links, malformed schemes, and missing optionals.
- [ ] **Step 2: Add failing migration/repository round-trip tests** proving legacy media and user-edited fields survive.
- [ ] **Step 3: Extend the external GraphQL query, DTO, parser, mapper, domain, schema, external SQL, and SQLite mapper**.
- [ ] **Step 4: Normalize synopsis paragraph breaks and deduplicate validated links outside QML**.
- [ ] **Step 5: Run GraphQL, repository, migration, sync, and home regression tests**.
- [ ] **Step 6: Run full CTest and commit** with `feat: persist extended media details`.
- [ ] **Review gate:** stop and obtain acceptance before Task 11.

### Task 11: Full media-details overlay

**Files:**
- Create: `resources/qml/MediaDetailsPanel.qml`
- Modify: `resources/qml/Home.qml`
- Modify: `src/presentation/home/HomeScreenController.h`
- Modify: `src/presentation/home/HomeScreenController.cpp`
- Modify: `tests/unit/HomeScreenControllerTests.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces presentation-ready selected-media fields for all spec-approved metadata and a validated list of `{site, url}` links.
- QML emits explicit link activation to `Qt.openUrlExternally` only for controller-approved URLs.

- [ ] **Step 1: Add failing controller tests** for complete, partial, long-synopsis, next-episode, and deduplicated-link presentations.
- [ ] **Step 2: Add QML checks** for `Ver detalhes`, synopsis activation, scrollability, selectable text, and three close paths.
- [ ] **Step 3: Implement controller properties and the left-side overlay** while keeping the compact details panel unchanged.
- [ ] **Step 4: Verify focus containment/restoration and that text selection/link inspection do not close the panel**.
- [ ] **Step 5: Validate wide/narrow layouts and missing fields**, then run focused and full tests.
- [ ] **Step 6: Commit** with `feat: add full media details`.
- [ ] **Review gate:** stop and obtain acceptance before Task 12.

### Task 12: Dedicated AniList seasonal catalog

**Files:**
- Create: `src/application/catalog/SeasonalCatalogTypes.h`
- Create: `src/application/catalog/ISeasonalCatalogDataSource.h`
- Create: `src/app/SeasonalCatalogCoordinator.h`
- Create: `src/app/SeasonalCatalogCoordinator.cpp`
- Create: `src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h`
- Create: `src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.cpp`
- Create: `src/presentation/catalog/SeasonalCatalogController.h`
- Create: `src/presentation/catalog/SeasonalCatalogController.cpp`
- Create: `resources/anilist/queries/seasonal-catalog.graphql`
- Create: `resources/qml/SeasonalCatalogScreen.qml`
- Create: focused coordinator/data-source/controller tests
- Modify: `ApplicationComposition.*`, `Main.qml`, CMake, GraphQL fixtures

**Interfaces:**
- `SeasonalCatalogRequest { int year; QString seasonKey; int page; int perPage; }`.
- `ISeasonalCatalogDataSource::Fetch(const SeasonalCatalogRequest &, MediaPage &, QString &error)`.
- Controller properties for available years/seasons, selected keys, state, results, page information, and error text.
- Controller invokables `SetYear(int)`, `SetSeason(QString)`, `LoadNextPage()`, `Retry()`, and `SelectMedia(int)`.

- [ ] **Step 1: Add failing request-gate tests** proving zero calls for no filters or one filter, and exactly one page-one call when both become valid.
- [ ] **Step 2: Add failing pagination/concurrency tests** for all-catalog results, duplicate IDs, stale responses, retry, empty page, and load-next-page bounds.
- [ ] **Step 3: Add failing GraphQL tests** for explicit year/season variables and external query loading.
- [ ] **Step 4: Implement the dedicated data source and coordinator** without reusing the user-list synchronization filter contract.
- [ ] **Step 5: Implement the controller and screen** with backend-provided selectors and idle/loading/populated/empty/error states.
- [ ] **Step 6: Wire top-level navigation and reuse preferred-title, cover, and full-details presentation** without automatically adding catalog results to the user's list.
- [ ] **Step 7: Validate no startup request, filter changes, stale-response rejection, paging, retry, and wide/narrow layouts**.
- [ ] **Step 8: Run all focused tests, full build, full CTest, and a monitored AniList smoke; commit** with `feat: add seasonal AniList catalog`.
- [ ] **Review gate:** stop and obtain final acceptance.

### Task 13: Whole-program regression and release evidence

**Files:**
- Modify only files required to correct regressions introduced by Tasks 1–12.

**Interfaces:**
- Consumes every previous task; produces release evidence, not new features.

- [ ] **Step 1: Build from a fresh CMake configuration** with the supported Qt/MinGW/Ninja toolchain.
- [ ] **Step 2: Run complete CTest** and require zero failures.
- [ ] **Step 3: Test migrations** from schema versions 8 and every version introduced by this plan, plus a fresh database.
- [ ] **Step 4: Restart twice** to verify sort, language, title preference, card-status preference, temporary cleanup, and scan logs.
- [ ] **Step 5: Perform wide/narrow visual regression** for settings, home, cover preview, details overlay, and seasonal catalog.
- [ ] **Step 6: Run `git diff --check` and a final whole-branch review** against the specification.
- [ ] **Step 7: Commit only verification-driven corrections** with `fix: complete frontend and catalog improvements`; skip if unnecessary.
