# SDD ledger — plan: docs/superpowers/plans/2026-10-03-library-small-corrections.md

Pre-flight: Task 1 has no shared interface dependency on later tasks; fixture remains read-only.

Task 1: complete (tests: direct QtTest executables → 5/5, 13/13, 13/13 pass)
- Added parser regression coverage for media 154587, asserting consumed and total chapters are 28.
- Extended flow coverage to assert 28/28 reaches the writer.
- Updated recorded user-list persistence coverage to assert media 154587 is stored as 28/28 with score 10 and Completed status.
- Ruling: no production correction was needed from source inspection — parser maps entry progress, mapper maps episodes/progress, and repository upsert binds both fields; cost if wrong: the new focused tests will expose any hidden runtime mismatch.
- Ruling: focused execution is blocked in this environment — the worktree has no configured build directory and CMake cannot find a C++ compiler; cost if wrong: test compilation/runtime issues remain unverified until a Qt/MSVC build environment is available.
- Verification update: configured `cmake-build-debug` with Visual Studio's Ninja generator, MSVC x64, and Qt 6.8.3 MSVC. The initial Visual Studio generator failed because MSBuild received duplicate `PATH`/`Path` environment keys; Ninja avoided that host-environment issue.

Task 2: complete (tests: HomeScreenControllerTests → 42/42 pass; QmlStructureTests → 18/18 pass, 1 skipped)
- Added `season_asc` and `season_desc` controller options using `(SeasonYear, seasonRank)` with `WINTER < SPRING < SUMMER < FALL` and deterministic zero fallback for missing values.
- Added controller regression coverage for year precedence, both directions, and missing season/year values.
- Kept sorting in C++; BrowseControls continues consuming controller-provided sort options.
- Aligned BrowseControls synchronization references with its root id so the existing structural contract is valid.

Task 3: complete (tests: HomeScreenControllerTests → 42/42 pass; QmlStructureTests → 18/18 pass, 1 skipped)
- Kept normal cards on controller-prepared translated metadata and made progress/score labels use the active Qt translation context.
- Added explicit compact metadata presentation for complete-library cards, preserving values while hiding progress and score labels and using compact status/progress/score metadata.
- Preserved the existing controller-owned season detail property and structural details presentation.

Task 4: complete (tests: QmlStructureTests → 20/20 pass, 1 skipped; HomeScreenControllerTests → 42/42 pass)
- Added focus-only clearing for browse controls and a Home-level outside-click focus path that does not call `ClearBrowseCriteria`.
- Changed the complete-library popup to close only through Escape or its explicit close button, preserving it during details interaction.
