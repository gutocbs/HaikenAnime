# Task 12 — Part 3 report

## Delivered

- Added a local-only seasonal personal-list service. It reads the configured local repository, refuses an absent-media save without a concrete `UserListStatus`, writes a new local record once, and treats repeated saves as idempotent reuse of that record.
- Extended the seasonal controller with membership, status-option, editor-value, save-error, and save-success contracts. Existing local records supply their saved progress, score, status, and alternative names; catalog metadata never replaces those user values during the reuse path.
- Reused `EditMediaPanel` for the seasonal details action. An absent record shows **Adicionar à minha lista** and opens with no preselected status; an existing record shows **Editar mídia**. The action stays open after validation or persistence errors and closes only after a successful controller save.
- Connected a successful first add to `HomeScreenController::reload`, so Home and its complete-list model read the new repository entry without an import or AniList mutation.
- No remote AniList list synchronization was added.

## Validation

- Focused `SeasonalCatalogControllerTests`, `SqliteRepositoryTests`, `SqliteDatabaseTests`, `SqliteQueryConfigurationTests`, `HomeScreenControllerTests`, `SettingsControllerTests`, and `QmlStructureTests` passed.
- `HaikenAnime` Release target built successfully.
- Full CTest ran 37 tests: 34 passed. `InitialSyncCoordinatorTests::synchronizesRealGraphQlFixtureIntoDatabase` timed out waiting for its completion signal; `LocalLibraryScanCoordinatorTests` and `LocalLibraryScanCompositionTests` failed without test output. None of those targets was touched by this task. Final whitespace validation passed.

## Scope notes

- The existing Home editor remains the owner of its existing edit-preview behavior. This task only uses that editor to require a first personal-list status for seasonal additions and to reuse existing-record values without creating duplicates.

## Follow-up correction

- The original existing-record save path reused the stored record without writing editor changes. It now carries progress, score, status, local path, and alternative names from the seasonal editor through the controller and a dedicated local repository update boundary.
- The SQLite schema now persists `local_path` (migration 15). The dedicated update only changes personal-list-owned fields, so catalog metadata and unrelated user fields such as `next_chapter` remain intact. The editor is prefilled with the saved path for existing records to avoid clearing an untouched value.
- Added controller and SQLite regressions for changed editor values/status, repeat-save cardinality, metadata preservation, query configuration packaging, and the migration path. Focused tests and the Release application build passed after the correction.
