# Task 7 report: scheduler and background synchronization composition

## Scope completed

- Deferred adaptive scheduling until the legacy initial synchronization reaches either its completion or failure boundary; the initial coordinator no longer owns a fixed recurring schedule.
- Kept scheduler execution asynchronous and startup-safe: the runtime initializes its worker resources first, then starts only after an explicit request.
- Seeded only the executable persisted task partitions (user list plus active, inactive, and completed catalog) from their configured adaptive policies. Existing persisted states, including checkpoints, remain authoritative.
- Routed user-list work to the recorded user-list source and catalog partitions to the recorded catalog source. A failed/incomplete run resumes at the next persisted page; a successful later cadence clears the checkpoint and restarts at page one.
- Kept cancellation/drain ownership in the existing runtime/coordinator shutdown order. Background task failures are now sent to existing status surfaces and the async logger without blocking startup.
- Did not add Settings UI/cache-cleanup wiring.

## Changed files

- `main.cpp`
- `src/app/AdaptiveSyncCoordinator.h`
- `src/app/AdaptiveSyncCoordinator.cpp`
- `src/app/AdaptiveSyncRuntime.h`
- `src/app/AdaptiveSyncRuntime.cpp`
- `src/app/ApplicationComposition.cpp`
- `src/app/ApplicationSyncTaskExecutor.h`
- `src/app/ApplicationSyncTaskExecutor.cpp`
- `tests/unit/AdaptiveSyncCoordinatorTests.cpp`
- `tests/unit/AdaptiveSyncRuntimeTests.cpp`

## TDD evidence and commands

1. RED: added `startsPersistedWorkOnlyAfterExplicitActivation` and ran:

   `cmake --build cmake-build-task6 --target AdaptiveSyncRuntimeTests --parallel 2`

   Expected compilation failure observed: `AdaptiveSyncRuntime` had no member `start`.

2. GREEN: after the first runtime implementation, ran:

   `cmake --build cmake-build-task6 --target AdaptiveSyncRuntimeTests --parallel 2; ctest --test-dir cmake-build-task6 --output-on-failure -R '^AdaptiveSyncRuntimeTests$'`

   Result: passed, `1/1`, 5.95 seconds.

3. RED: added `persistsMissingPolicyPartitionsBeforeSchedulingThem` and ran:

   `cmake --build cmake-build-task6 --target AdaptiveSyncCoordinatorTests --parallel 2; ctest --test-dir cmake-build-task6 --output-on-failure -R '^AdaptiveSyncCoordinatorTests$'`

   Result: expected test failure before seeding behavior was implemented.

4. Static validation after the final edits:

   `git diff --check`

   Result: passed (exit code 0).

5. Clean build setup:

   `cmake -S . -B cmake-build-task7 -G Ninja -DCMAKE_MAKE_PROGRAM='C:\Users\gutoc\AppData\Local\Programs\CLion\bin\ninja\win\x64\ninja.exe' -DCMAKE_CXX_COMPILER='C:\Qt\Tools\mingw1310_64\bin\g++.exe' -DCMAKE_BUILD_TYPE=Debug`

   Result: configured successfully. The final clean build compiled the changed application units (`main.cpp`, `ApplicationComposition.cpp`, `AdaptiveSyncCoordinator.cpp`, `AdaptiveSyncRuntime.cpp`, and `ApplicationSyncTaskExecutor.cpp`) before the user requested that all builds stop.

## Unverified checks

- Final focused runtime/coordinator test execution after all final edits: unverified. The inherited `cmake-build-task6` execution exited with code 2 from its build directory and emitted no test diagnostic; the fresh build was deliberately stopped by user request before link/test completion.
- Full CTest suite, final application link, manual QML/window startup, and runtime shutdown under a real background synchronization run: unverified.

## Concerns

- The application currently uses offline recorded GraphQL resources for the composed background sources, preserving fixture/offline startup. Live AniList transport ownership remains outside this task.
- Existing unit-test source contains prior `[[nodiscard]]` warnings for ignored `AdaptiveSyncCoordinator::Start()` results. They are pre-existing warning noise, not introduced production compiler errors.
- The pre-existing `cmake-build-task6` directory reported `ninja: warning: premature end of file; recovering` after interrupted builds. It should not be used as final test evidence; recreate or clean it before the next validation pass.
