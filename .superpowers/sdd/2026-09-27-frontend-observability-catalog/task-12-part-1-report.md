# Task 12 — Part 1 Report: Seasonal Catalog Backend Data Source

## Delivered scope

Implemented the backend-only seasonal AniList catalog boundary. This part deliberately has no QML, controller registration, application-composition wiring, navigation, personal-list add flow, details-editor work, startup request, or live AniList smoke request.

## Changed files

- `src/application/catalog/SeasonalCatalogTypes.h`
- `src/application/catalog/ISeasonalCatalogDataSource.h`
- `src/app/SeasonalCatalogCoordinator.h`
- `src/app/SeasonalCatalogCoordinator.cpp`
- `src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h`
- `src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.cpp`
- `resources/anilist/queries/seasonal-catalog.graphql`
- `tests/fixtures/graphql/seasonal-complete.json`
- `tests/fixtures/graphql/seasonal-empty.json`
- `tests/fixtures/graphql/seasonal-malformed.json`
- `tests/fixtures/graphql/seasonal-page-two.json`
- `tests/unit/SeasonalCatalogTests.cpp`
- `CMakeLists.txt`

## Contracts and behavior

- `SeasonalCatalogRequest` carries explicit `year`, stable season key, `page`, and `perPage`. Valid keys are `WINTER`, `SPRING`, `SUMMER`, and `FALL`; page sizes are bounded to 50.
- `ISeasonalCatalogDataSource::Fetch` is separate from `IMediaDataSource` and does not use `MediaSyncFilter`, so it cannot inherit the user-list synchronization contract.
- `GraphQlSeasonalCatalogDataSource` loads `:/anilist/queries/seasonal-catalog.graphql`, sends explicit `year`, `season`, `page`, and `perPage` GraphQL variables, parses the existing page envelope, and returns GraphQL errors through its existing `QString` error contract.
- `SeasonalCatalogCoordinator` remains idle with no default selector values. It fetches only when both year and season are valid, validates returned pagination metadata, bounds next-page calls, rejects stale reentrant responses by generation, and retains first occurrence order while dropping duplicate IDs within or across pages.
- The dedicated query uses only selected season/year and page variables; it does not include AniList user-list fields or local-library filters.

## Fixtures and focused coverage

- Added complete, empty, malformed-pagination, and second-page JSON fixtures under `tests/fixtures/graphql/`.
- Reused the existing `error-response.json` GraphQL fixture for error propagation.
- `SeasonalCatalogTests` covers validation/gating, no startup/default request, pagination envelope bounds, duplicate IDs, empty pages, stale success/error rejection, resource query loading, exact JSON request variables through a local `QTcpServer`, parsed catalog output, malformed responses, and GraphQL error propagation.

## Validation

Commands and fresh outcomes:

```powershell
cmake --build cmake-build-release --target SeasonalCatalogTests --config Release
ctest --test-dir cmake-build-release -R '^SeasonalCatalogTests$' --output-on-failure
```

The seasonal target built successfully and `SeasonalCatalogTests` passed (1/1).

```powershell
cmake --build cmake-build-release --config Release
```

The complete Release build succeeded (exit code 0).

```powershell
ctest --test-dir cmake-build-release --output-on-failure
```

The complete suite ran 36 tests: 34 passed and 2 executables failed, both outside this task.

- `LocalLibraryScanCoordinatorTests`: its log-lifecycle assertions expect completion strings not present in the produced log.
- `LocalLibraryScanCompositionTests`: the direct verbose run also showed a pre-existing configuration-log assertion failure and then a fatal `Cannot write fixture` while recreating `cover-composition-smoke.tmp`.

No unrelated fixes were made.

## Limitations and deferred work

- Part 2 must expose this coordinator through a presentation controller and wire it into composition/navigation without creating a startup request.
- The user-list add path, details-editor changes, screen, and end-to-end/live AniList validation remain intentionally deferred.
- The full CTest suite is not clean because of the unrelated local-library scan failures above; focused seasonal validation and the full build are clean.
