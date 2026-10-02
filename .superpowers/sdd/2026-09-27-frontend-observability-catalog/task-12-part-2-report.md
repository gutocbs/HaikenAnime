# Task 12 — Part 2 report

## Delivered

- Added `SeasonalCatalogController` and its `SeasonalCatalogMediaModel`. The controller exposes C++-owned year and season options, empty initial selections, coordinator state/error/paging, preferred-title and cover-quality presentation, and the existing read-only details-panel property contract.
- Registered the seasonal GraphQL client, query store, data source, and coordinator in `ApplicationComposition`; `main.cpp` exposes the controller to QML and propagates presentation preference changes.
- Added a separate `SeasonalCatalogScreen.qml` and the Home-to-seasonal navigation boundary. No seasonal fetch is initiated during application startup; the controller delegates only explicit valid two-filter requests to the coordinator.
- Added focused controller and QML/navigation tests. The screen has loading, empty, error/retry, populated, next-page, selection, and wide/narrow grid states. It has no list-inclusion or edit/save controls.

## Decisions

- The filters are intentionally unselected initially. `ComboBox.currentIndex` remains `-1` until the coordinator has an explicit selected year or season, so selecting only one filter cannot issue a request.
- Result metadata stays in the controller/model: QML receives prepared preferred titles, cover sources, labels, and the established details properties. The screen reuses `MediaCard` and `MediaDetailsPanel`; it does not parse media metadata or implement an editor.
- The composition owns the network manager before the GraphQL client, data source, and coordinator, preserving their borrowed-dependency lifetime order.

## Validation

Commands run:

```powershell
cmake --build cmake-build-release --target HaikenAnime SeasonalCatalogControllerTests QmlStructureTests --parallel 4
ctest --test-dir cmake-build-release --output-on-failure -R "SeasonalCatalogTests|SeasonalCatalogControllerTests|QmlStructureTests"
ctest --test-dir cmake-build-release --output-on-failure
git diff --check
```

Focused validation passed: `SeasonalCatalogTests`, `SeasonalCatalogControllerTests`, and `QmlStructureTests` all passed (including the explicit-filter gate, state/error/retry/paging, selection, and responsive/navigation source contracts). The application target built and Qt's QML deployment scanned `SeasonalCatalogScreen.qml` successfully.

The full CTest run completed with 35/37 passing. Existing logging/fixture tests remain red outside this part:

- `LocalLibraryScanCoordinatorTests`: three assertions do not observe expected asynchronous logger entries.
- `LocalLibraryScanCompositionTests`: an existing stored-preference logger assertion fails, then the fixture setup terminates with `Cannot write fixture`.

The composition target was recompiled with the new controller and dependencies before that full run.

## Visual validation

QML compilation and structure checks passed, but no interactive desktop walkthrough was run in this non-interactive validation environment. In particular, live AniList responses, remote cover rendering, and manual wide/narrow resizing remain unverified here.
