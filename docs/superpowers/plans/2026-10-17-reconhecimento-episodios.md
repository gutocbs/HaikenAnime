# Reconhecimento de episódios por plugins Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement automatic episode recognition for mpv and Plex, updating only unseen episodes after a configurable continuous playback interval and persisting history for correction and synchronization.

**Architecture:** Keep player integrations behind internal plugin ports in `application`, with mpv IPC and Plex HTTP/authentication adapters in `infrastructure`. A central observation/decision pipeline owns timing, deduplication, progress changes, history, and AniList synchronization; Qt/QML controllers expose plugin schemas and recognition records without owning domain rules.

**Tech Stack:** C++/Qt 6, QML, CMake, SQLite migrations and repositories, Qt Network/NetworkAuth, Windows named pipes, Plex HTTP API, AniList existing pending-change synchronization, CTest.

**Spec:** `docs/superpowers/specs/2026-10-17-reconhecimento-episodios-design.md`

## Global Constraints

- Initial platform is Windows; Crunchyroll, Plex Web, external dynamic plugins, and generic external mpv discovery remain out of scope.
- mpv is launched by HaikenAnime with `--input-ipc-server`; an external mpv without IPC is not automatically recognized.
- The default minimum continuous playback time is three minutes and is user-configurable without recompilation.
- Only episodes not already watched may change local progress automatically.
- Plex sessions are filtered by explicitly selected users.
- QML only presents state; player parsing, timing, progress decisions, persistence, and AniList calls stay outside QML.
- Tokens and other secrets must not be returned to QML after storage or written to logs.
- New public C++ code uses the existing V2 responsibility boundaries and English names.
- Every completed behavior receives unit tests under `tests/unit/`; integration tests use doubles, temporary databases, or fixtures.

## Review Focus

- A playback session that closes at 2:59 must not change progress — pinned by `PlaybackObservationServiceTests`.
- A repeated poll for the same Plex session must not create duplicate progress changes — pinned by `EpisodeRecognitionServiceTests`.
- A Plex session from an unselected user must be ignored — pinned by `PlexPluginTests`.
- A missing/invalid Plex token must not leak credentials or prevent application startup — pinned by `PlexAuthenticationTests` and composition tests.
- A saved dynamic secret must be masked and never emitted in schema values/log output — pinned by `PluginSettingsServiceTests`.

## File Map

Create focused modules under:

- `src/domain/recognition/`: value objects and immutable domain records.
- `src/application/recognition/`: plugin ports, observation, decision, history, and settings services.
- `src/infrastructure/recognition/`: SQLite repositories, mpv IPC, Plex HTTP/authentication, and plugin implementations.
- `src/presentation/recognition/` and `src/presentation/settings/`: controllers/models and integration with the existing settings composition.
- `resources/qml/`: recognition history and dynamic plugin settings screens.
- `resources/sqlite/migrations/` and `resources/sqlite/queries/`: schema and SQL resources.
- `tests/unit/` and `tests/fixtures/`: isolated behavior tests and Plex/mpv payloads.
- `CMakeLists.txt`: register all new production and test sources/resources.

Do not extend the legacy `src/utilities/playerrecognition.*` into the new pipeline; leave it as a compatibility reference until callers are migrated and a separate cleanup is approved.

### Task 1: Domain records and recognition policy

**Files:**
- Create: `src/domain/recognition/PlaybackSession.h/.cpp`
- Create: `src/domain/recognition/EpisodeCandidate.h/.cpp`
- Create: `src/domain/recognition/RecognitionRecord.h/.cpp`
- Create: `src/domain/recognition/RecognitionPolicy.h/.cpp`
- Test: `tests/unit/RecognitionDomainTests.cpp`

**Interfaces:**
- Produce `PlaybackSessionId`, `PlaybackState`, `EpisodeCandidate`, `RecognitionStatus`, and `RecognitionPolicy::minimumPlaybackSeconds()` with validated value semantics.
- `RecognitionPolicy` accepts a positive duration and defaults to `180` seconds.

- [ ] Write tests for valid/invalid duration, session identity, candidate confidence, recognition status transitions, and equality/deduplication keys.
- [ ] Run `ctest --test-dir <build> -R RecognitionDomainTests` and confirm the new tests initially fail.
- [ ] Implement the domain records without Qt network, SQLite, or QML dependencies.
- [ ] Run the focused test again and confirm PASS.
- [ ] Commit `feat: add episode recognition domain records`.

### Task 2: Plugin ports, schemas, and settings service

**Files:**
- Create: `src/application/recognition/IPlaybackPlugin.h`
- Create: `src/application/recognition/IPluginSettingsRepository.h`
- Create: `src/application/recognition/PluginSettingsSchema.h/.cpp`
- Create: `src/application/recognition/PluginSettingsService.h/.cpp`
- Create/modify: `src/infrastructure/persistence/sqlite/SqlitePluginSettingsRepository.h/.cpp`
- Create: `resources/sqlite/migrations/017-create-recognition-settings.sql`
- Create: `resources/sqlite/queries/read-plugin-settings.sql`, `upsert-plugin-settings.sql`
- Test: `tests/unit/PluginSettingsServiceTests.cpp`, `tests/unit/SqlitePluginSettingsRepositoryTests.cpp`

**Interfaces:**
- `IPlaybackPlugin::id()`, `displayName()`, `capabilities()`, `settingsSchema()`, `observe(...)`, `testConnection()`, and `stop()`.
- `PluginSettingsService::schemas()`, `values(pluginId)`, `setValue(pluginId, key, value)`, `save(pluginId)`, `clearSecret(pluginId, key)`.
- `PluginSettingField` contains key, type, label, description, required, secret, default value, and validation metadata.

- [ ] Write tests for typed fields, defaults, required validation, secret masking, persistence round-trip, and unknown plugin keys.
- [ ] Run focused tests and verify failure before implementation.
- [ ] Add the migration and repository using temporary SQLite databases and existing query/resource conventions.
- [ ] Implement the service so secret values are write-only to presentation and never included in logs.
- [ ] Run focused tests and `git diff --check`.
- [ ] Commit `feat: add dynamic plugin settings infrastructure`.

### Task 3: Playback observation and automatic progress decision

**Files:**
- Create: `src/application/recognition/PlaybackObservationService.h/.cpp`
- Create: `src/application/recognition/ProgressDecisionService.h/.cpp`
- Create/modify: existing media/progress application port identified from current composition
- Test: `tests/unit/PlaybackObservationServiceTests.cpp`, `tests/unit/ProgressDecisionServiceTests.cpp`

**Interfaces:**
- `PlaybackObservationService::start(PlaybackSession, EpisodeCandidate)`, `update(sessionId, PlaybackSnapshot)`, `stop(sessionId, StopReason)`.
- `ProgressDecisionService::evaluate(EpisodeCandidate) -> ProgressDecision`.
- `ProgressDecision` distinguishes `AlreadyWatched`, `MarkWatched`, `Uncertain`, and `Ignored`.

- [ ] Write fake-clock tests proving completion occurs only after 180 seconds by default, configurable duration, continuous session validity, pause/close handling, and duplicate completion suppression.
- [ ] Write decision tests proving an already watched episode is never regressed and an unseen episode produces one progress change.
- [ ] Run focused tests to verify failure.
- [ ] Implement timer/state handling with an injected clock and repository/application ports, never wall-clock sleeps in tests.
- [ ] Run focused tests and confirm PASS.
- [ ] Commit `feat: add automatic playback observation policy`.

### Task 4: Recognition history and retryable AniList handoff

**Files:**
- Create: `src/application/recognition/IRecognitionHistoryRepository.h`
- Create: `src/application/recognition/RecognitionHistoryService.h/.cpp`
- Create: `src/infrastructure/persistence/sqlite/SqliteRecognitionHistoryRepository.h/.cpp`
- Create: `resources/sqlite/migrations/018-create-recognition-history.sql`
- Create: `resources/sqlite/queries/read-recognition-history.sql`, `insert-recognition-record.sql`, `update-recognition-record.sql`
- Test: `tests/unit/RecognitionHistoryServiceTests.cpp`, `tests/unit/SqliteRecognitionHistoryRepositoryTests.cpp`

**Interfaces:**
- `RecognitionHistoryService::record(...)`, `list(filter, page)`, `correct(recordId, EpisodeCandidate)`, `ignore(recordId)`, `markSyncFailure(recordId, error)`.
- Repository records source plugin, session/user/device identity, candidate, confidence, status, timestamps, correction and sync state.

- [ ] Write tests for pending/completed/ignored/corrected states, durable sync failures, pagination, and duplicate session keys.
- [ ] Run focused tests to verify failure.
- [ ] Implement migration/repository with temporary databases and preserve local progress when AniList synchronization fails.
- [ ] Connect `MarkWatched` to the existing pending-change/AniList synchronization boundary rather than calling AniList from recognition code.
- [ ] Run focused tests plus the existing AniList pending-change tests.
- [ ] Commit `feat: persist episode recognition history`.

### Task 5: mpv IPC adapter and plugin

**Files:**
- Create: `src/infrastructure/recognition/mpv/MpvIpcClient.h/.cpp`
- Create: `src/infrastructure/recognition/mpv/MpvPlugin.h/.cpp`
- Modify: mpv launch/application composition files found during implementation
- Create: `tests/fixtures/mpv-playing.json`, `tests/fixtures/mpv-shutdown.json`
- Test: `tests/unit/MpvIpcClientTests.cpp`, `tests/unit/MpvPluginTests.cpp`

**Interfaces:**
- `MpvIpcClient::connect(pipeName)`, `getProperty(name)`, `observeProperty(id, name)`, `disconnected`, and normalized playback signals.
- `MpvPlugin::launchEpisode(path, LaunchOptions) -> PlaybackSession` must include `--input-ipc-server=<named-pipe>`.

- [ ] Write protocol tests using a fake named-pipe transport for title/path, `time-pos`, pause, property changes, malformed JSON, and shutdown.
- [ ] Write launch tests asserting the IPC argument and session identity.
- [ ] Run focused tests to verify failure.
- [ ] Implement the adapter using Windows named-pipe support behind a replaceable transport; reject sessions without IPC.
- [ ] Connect normalized events to `PlaybackObservationService` and test close-before-threshold behavior end-to-end with fakes.
- [ ] Run focused tests and a Windows compile of the target.
- [ ] Commit `feat: integrate mpv through local ipc`.

### Task 6: Plex authentication, server discovery, and session plugin

**Files:**
- Create: `src/infrastructure/recognition/plex/PlexApiClient.h/.cpp`
- Create: `src/infrastructure/recognition/plex/PlexAuthentication.h/.cpp`
- Create: `src/infrastructure/recognition/plex/PlexPlugin.h/.cpp`
- Create: `tests/fixtures/plex-sessions.json`, `plex-resources.json`, `plex-users.json`
- Test: `tests/unit/PlexAuthenticationTests.cpp`, `tests/unit/PlexApiClientTests.cpp`, `tests/unit/PlexPluginTests.cpp`

**Interfaces:**
- `PlexAuthentication::beginBrowserLogin()`, `pollLogin()`, `disconnect()`.
- `PlexApiClient::discoverServers(token)`, `listUsers(server)`, `listSessions(server, token)`.
- `PlexPlugin::selectedUsers()`, `setSelectedUsers()`, and `pollSessions()`.

- [ ] Write HTTP-double tests for PIN polling success/timeout, token masking, server discovery, session parsing, missing fields, and unauthorized responses.
- [ ] Write plugin tests for selected-user filtering, multi-user identity preservation, deduplication, and unavailable server behavior.
- [ ] Run focused tests to verify failure.
- [ ] Implement browser launch through the existing Windows URL launcher pattern and keep token access inside infrastructure/secret storage.
- [ ] Implement Plex session polling with an injectable interval/timer and normalized `EpisodeCandidate` output.
- [ ] Run focused tests and verify no credentials appear in captured logs.
- [ ] Commit `feat: add plex authentication and session plugin`.

### Task 7: Plugin composition and application lifecycle

**Files:**
- Create: `src/application/recognition/PluginRegistry.h/.cpp`
- Create: `src/application/recognition/EpisodeRecognitionService.h/.cpp`
- Modify: `src/app/` composition/context files and `CMakeLists.txt`
- Test: `tests/unit/PluginRegistryTests.cpp`, `tests/unit/EpisodeRecognitionServiceTests.cpp`, `tests/unit/RecognitionCompositionTests.cpp`

**Interfaces:**
- `PluginRegistry::registerPlugin(unique_ptr<IPlaybackPlugin>)`, `plugins()`, `plugin(id)`.
- `EpisodeRecognitionService::start()`, `stop()`, and plugin event handling that routes candidates through observation, decision, history, and pending AniList change services.

- [ ] Write composition tests proving the application starts with no Plex server, no mpv instance, or invalid plugin settings.
- [ ] Write pipeline tests for selected Plex user, unseen episode, already watched episode, low confidence, duplicate session, and AniList failure.
- [ ] Run focused tests to verify failure.
- [ ] Register mpv and Plex plugins in application composition without recreating a monolithic `MainClass`.
- [ ] Implement lifecycle shutdown so IPC/network polling and timers stop cleanly.
- [ ] Run focused tests, full CTest, and `git diff --check`.
- [ ] Commit `feat: compose episode recognition plugins`.

### Task 8: Dynamic plugin settings and recognition history UI

**Files:**
- Create: `src/presentation/settings/PluginSettingsController.h/.cpp`
- Create: `src/presentation/recognition/EpisodeRecognitionController.h/.cpp`
- Create: `src/presentation/recognition/RecognitionListModel.h/.cpp`
- Create: `resources/qml/PluginSettingsScreen.qml`, `PluginSettingField.qml`, `EpisodeRecognitionScreen.qml`
- Modify: `resources/qml/SettingsScreen.qml`, `resources/qml/Main.qml`, resource registration
- Test: `tests/unit/PluginSettingsControllerTests.cpp`, `tests/unit/EpisodeRecognitionControllerTests.cpp`, `tests/unit/QmlRecognitionStructureTests.cpp`

**Interfaces:**
- `PluginSettingsController` exposes plugin list, schema fields, masked values, dirty/valid/saving state, `save`, `discard`, `connect`, `disconnect`, `testConnection`, and `selectUsers`.
- `EpisodeRecognitionController` exposes current sessions, history model, errors, `correct(recordId, candidate)`, and `ignore(recordId)`.

- [ ] Write controller tests for schema rendering data, invalid required fields, secret masking, save/discard, connection state, correction, and ignore actions.
- [ ] Add QML structure tests proving no player parsing, timers, direct AniList calls, or raw token display occur in QML.
- [ ] Run focused tests to verify failure.
- [ ] Implement generic controls for text, number, boolean, selection, secret, and plugin actions; keep Plex/mpv-specific behavior in controller/plugin capabilities.
- [ ] Add the recognition screen as a history/status panel, not a confirmation queue.
- [ ] Run focused controller/QML tests and perform a manual Windows visual check.
- [ ] Commit `feat: add plugin settings and recognition history screens`.

### Task 9: End-to-end validation and delivery review

**Files:**
- Modify: `tests/integration/` or add integration harness files selected by existing test conventions
- Modify: `docs/superpowers/validation/2026-10-17-reconhecimento-episodios.md`

- [ ] Run the focused unit tests for domain, settings, observation, history, mpv, Plex, composition, controllers, and QML structure.
- [ ] Run complete CTest and record unrelated failures separately from feature failures.
- [ ] On Windows, launch an episode through the configured mpv adapter and verify IPC recognition, three-minute threshold behavior, history, and local progress; label live checks explicitly.
- [ ] Against a test Plex server/account, verify browser login, selected-user filtering, active-session recognition, and unavailable-server recovery; label as unverified if credentials/server are unavailable.
- [ ] Run `git diff --check`, inspect the full diff, and verify no secrets or generated build files are included.
- [ ] Commit validation evidence or report it without claiming unverified paths as passing.
