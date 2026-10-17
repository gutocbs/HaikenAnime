# Tela "Assistindo agora" Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Show, while an episode is being recognized, the anime (from the local database) and the episode details the plugin already has, behind one "Assistindo agora" button to the right of "Configurações".

**Architecture:** Plugins attach optional `EpisodeDetails` to `PlaybackSession` and may push updates through a new `IPlaybackObserver::onSessionDetailsChanged` (default no-op). `EpisodeRecognitionService` feeds a new `ActiveSessionRegistry` with the events it already receives; `NowWatchingController` reads the registry, joins the anime from `IMediaReader`, and exposes a selectable session to QML.

**Tech Stack:** C++20/Qt 6.8 (Core, Qml, Quick), QtTest/CTest, CMake + Ninja with MSVC (`cmake-build-validation`).

**Spec:** `docs/superpowers/specs/2026-10-17-assistindo-agora-design.md`

## Global Constraints

- The HaikenAnime only displays episode data the plugin already received; no new network calls or metadata lookups.
- No episode image; the card uses the anime cover from the database or the existing placeholder.
- One "Assistindo agora" button, placed to the **right** of "Configurações", visible only while at least one session is active; no counter on the button.
- With more than one active session, the session is chosen inside the screen; the screen shows one session at a time; the default is the most recent; when the selected session ends, selection moves to the next active one.
- The screen is informational: no progress bar, playback state, predicted outcome or actions.
- Without sessions the screen shows exactly `Nada sendo reproduzido no momento` and does not navigate back by itself.
- Unmatched anime shows the label exactly `não associado à biblioteca`.
- QML: no `Timer`, network/AniList calls, player parsing or token display; use `AppButton` (no native `Button`); handlers that read `checked`/`index` use `function() { … }` (existing `QmlStructureTests` rule).
- Visual: existing palette (`ink #172033`, `muted #68758a`, `line #d7dee8`, `surfaceSoft #f6f8fb`, `accent #315d91`), cards with radius 7; refine the screen with the `/frontend-design` skill inside this pattern.
- Recognition policy, progress decisions and history behavior must not change.

## Review Focus

- Details arriving for a session that already ended (late Plex poll, late mpv property) must not resurrect it — pinned by `ActiveSessionRegistryTests::ignoresDetailsForUnknownSession`.
- The same session switching episode (Plex autoplay) must replace the card, not add a second one — pinned by `ActiveSessionRegistryTests::sameSessionNewEpisodeReplacesEntry`.
- A Plex `summary`/`originallyAvailableAt` that is missing or not a string must produce an empty field, never "undefined" — pinned by `PlexApiClientTests::missingEpisodeDetailsStayEmpty` and `NowWatchingControllerTests::omitsMissingEpisodeFields`.
- The selected session ending while others remain must move the selection, and the last one ending must leave `active == false` with an empty `current` — pinned by `NowWatchingControllerTests::selectionMovesWhenSelectedSessionEnds`.
- Recognition stopping (app shutdown, plugin restart) must clear the screen and hide the button — pinned by `EpisodeRecognitionServiceTests::stopClearsActiveSessions`.

---

### Task 1: EpisodeDetails and the details observer hook

**Files:**
- Modify: `src/domain/recognition/PlaybackSession.h`
- Modify: `src/application/recognition/IPlaybackPlugin.h`
- Test: `tests/unit/RecognitionDomainTests.cpp`

**Interfaces:**
- Produces: `struct EpisodeDetails { QString sourceTitle; QString title; QString summary; std::optional<int> durationSeconds; std::optional<QDate> airDate; bool operator==(const EpisodeDetails&) const = default; }` in `PlaybackSession.h`; new member `EpisodeDetails details;` at the end of `PlaybackSession`.
- Produces: `virtual void IPlaybackObserver::onSessionDetailsChanged(const PlaybackSessionId &sessionId, const EpisodeDetails &details) {}` (non-pure, empty default).

- [ ] **Step 1: Write the failing test** `RecognitionDomainTests::episodeDetailsDefaultEmptyAndCompareByValue`: a default `EpisodeDetails` has empty strings and no duration/date; two details with the same `title`, `summary`, `durationSeconds = 1440`, `airDate = QDate(1999,10,20)` compare equal; changing `summary` makes them differ; a default `PlaybackSession{.id = …}` has `details == EpisodeDetails{}`.
- [ ] **Step 2: Run** `.superpowers/sdd/<plan>/run-tests.sh red1 RecognitionDomainTests` (build fails: `EpisodeDetails` undefined). Expected: FAIL.
- [ ] **Step 3: Add `EpisodeDetails`, the `details` member and the observer hook** as in Interfaces. Existing designated initializers keep compiling because `details` is last.
- [ ] **Step 4: Run** the domain tests plus `MpvPluginTests PlexPluginTests EpisodeRecognitionServiceTests` to prove existing observers still compile. Expected: all PASS.
- [ ] **Step 5: Commit** `feat: add episode details to playback sessions`.

### Task 2: ActiveSessionRegistry fed by the pipeline

**Files:**
- Create: `src/application/recognition/ActiveSessionRegistry.h/.cpp`
- Modify: `src/application/recognition/EpisodeRecognitionService.h/.cpp`
- Modify: `CMakeLists.txt` (add the new `.cpp` to `RECOGNITION_PIPELINE_SOURCES` and to the `HaikenAnime` `target_sources` recognition block; add `ActiveSessionRegistryTests`)
- Test: `tests/unit/ActiveSessionRegistryTests.cpp`, `tests/unit/EpisodeRecognitionServiceTests.cpp`

**Interfaces:**
- Consumes: `EpisodeDetails`, `IPlaybackObserver::onSessionDetailsChanged` (Task 1).
- Produces:
  - `struct ActiveSession { PlaybackSession session; EpisodeCandidate candidate; };`
  - `class ActiveSessionRegistry` with `void start(const PlaybackSession&, const EpisodeCandidate&)`, `void updateDetails(const PlaybackSessionId&, const EpisodeDetails&)`, `void stop(const PlaybackSessionId&)`, `void clear()`, `[[nodiscard]] QList<ActiveSession> sessions() const` (ordered by `session.startedAt`, then insertion), `void setChangedCallback(std::function<void()>)`.
  - `const ActiveSessionRegistry &EpisodeRecognitionService::activeSessions() const;`

- [ ] **Step 1: Write the failing tests** in `ActiveSessionRegistryTests`: `startAddsSessionAndNotifies` (one session, callback called once); `updateDetailsReplacesDetailsAndNotifies`; `stopRemovesSessionAndNotifies`; `keepsSeveralSessionsInStartOrder` (two keys, `startedAt` 20:00 and 20:01 → that order); `sameSessionNewEpisodeReplacesEntry` (same id, episode 1 then 2 → one entry with episode 2); `ignoresDetailsForUnknownSession` (no entry, no callback); `clearRemovesAllAndNotifiesOnce`.
- [ ] **Step 2: Write the failing tests** in `EpisodeRecognitionServiceTests`: `startedSessionsAppearWithResolvedCandidate` (Plex "One Piece" without mediaId → registry entry has `mediaId == 21`); `detailsChangesReachRegistry` (plugin calls `onSessionDetailsChanged` → entry title updated); `stoppedSessionLeavesRegistry`; `stopClearsActiveSessions` (after `service->stop()` registry is empty and late `onSessionStarted` adds nothing).
- [ ] **Step 3: Run** both suites. Expected: FAIL (registry missing).
- [ ] **Step 4: Implement `ActiveSessionRegistry`** with a `QList<ActiveSession>` keyed by `session.id.key()`; callback invoked after each effective change only.
- [ ] **Step 5: Feed it from `EpisodeRecognitionService`**: `onSessionStarted` → `registry.start(session, resolved)` using the same resolved candidate passed to observation; `onSessionStopped` → `registry.stop`; override `onSessionDetailsChanged` → `registry.updateDetails`; `stop()` → `registry.clear()` after plugins stop; events while `!running_` are ignored (existing guard).
- [ ] **Step 6: Run** `ActiveSessionRegistryTests EpisodeRecognitionServiceTests RecognitionCompositionTests`. Expected: PASS.
- [ ] **Step 7: Commit** `feat: track active recognition sessions`.

### Task 3: Plex episode details

**Files:**
- Modify: `src/infrastructure/recognition/plex/PlexApiClient.h/.cpp` (`PlexSession` gains `QString summary; std::optional<QDate> airDate;`)
- Modify: `src/infrastructure/recognition/plex/PlexPlugin.h/.cpp`
- Modify: `tests/fixtures/plex-sessions.json` (first episode gains `"summary": "Luffy sets sail."` and `"originallyAvailableAt": "1999-10-20"`)
- Test: `tests/unit/PlexApiClientTests.cpp`, `tests/unit/PlexPluginTests.cpp`

**Interfaces:**
- Consumes: `EpisodeDetails`, `onSessionDetailsChanged` (Task 1).
- Produces: `PlexSession::summary`, `PlexSession::airDate` (parsed with `QDate::fromString(value, Qt::ISODate)`, invalid → `std::nullopt`); `PlexPlugin::TrackedSession` also stores the last `EpisodeDetails`.

- [ ] **Step 1: Write the failing tests**: `PlexApiClientTests::parsesEpisodeSummaryAndAirDate` (fixture first session: `summary == "Luffy sets sail."`, `airDate == QDate(1999,10,20)`); `PlexApiClientTests::missingEpisodeDetailsStayEmpty` (second session: empty summary, no airDate; a session with `"originallyAvailableAt": 42` → no airDate).
- [ ] **Step 2: Write the failing tests**: `PlexPluginTests::startedSessionCarriesEpisodeDetails` (details: `sourceTitle "One Piece"`, `title "Romance Dawn"`, `summary`, `durationSeconds 1440`, `airDate`); `PlexPluginTests::changedDetailsAreReportedOnce` (poll 1 summary "A", poll 2 summary "B", poll 3 summary "B" → exactly one `onSessionDetailsChanged` with "B"); `PlexPluginTests::newEpisodeCarriesItsOwnDetails`.
- [ ] **Step 3: Run** both suites. Expected: FAIL.
- [ ] **Step 4: Implement** the two `PlexSession` fields and fill `PlaybackSession.details` on start (`durationSeconds = durationMs / 1000` only when `durationMs > 0`); on later polls of the same session and episode, call `onSessionDetailsChanged` only when the details differ from the tracked ones.
- [ ] **Step 5: Run** `PlexApiClientTests PlexPluginTests`. Expected: PASS.
- [ ] **Step 6: Commit** `feat: report plex episode details`.

### Task 4: mpv episode details

**Files:**
- Modify: `src/infrastructure/recognition/mpv/MpvPlugin.h/.cpp`
- Test: `tests/unit/MpvPluginTests.cpp`

**Interfaces:**
- Consumes: `EpisodeDetails`, `onSessionDetailsChanged` (Task 1).
- Produces: new observer ids `MediaTitleObserverId = 4`, `DurationObserverId = 5`; `PlaybackSession.details.sourceTitle` set to the file name at launch.

- [ ] **Step 1: Write the failing tests**: `observesMediaTitleAndDurationAfterConnecting` (observed properties are exactly `pause`, `time-pos`, `path`, `media-title`, `duration` in that order); `launchedSessionCarriesFileNameAsSourceTitle`; `mediaTitleAndDurationBecomeDetails` (events `media-title "One Piece 01"` then `duration 1420.6` → two `onSessionDetailsChanged`, the last with `title "One Piece 01"` and `durationSeconds 1420`); `repeatedDetailsAreNotReported` (same `media-title` twice → one report); `detailsAfterSessionEndAreIgnored`.
- [ ] **Step 2: Run** `MpvPluginTests`. Expected: FAIL.
- [ ] **Step 3: Implement**: observe the two properties in `onConnected`; keep current `EpisodeDetails` in `ActiveSession`; on change report through the observer only when the value differs; ignore non-string/non-number data.
- [ ] **Step 4: Run** `MpvPluginTests MpvIpcClientTests`. Expected: PASS.
- [ ] **Step 5: Commit** `feat: report mpv episode details`.

### Task 5: NowWatchingController and app wiring

**Files:**
- Create: `src/presentation/recognition/NowWatchingController.h/.cpp`
- Modify: `main.cpp` (create the controller next to `recognitionController`; context property `nowWatchingController`, `nullptr` when recognition is unavailable)
- Modify: `CMakeLists.txt` (`HaikenAnime` sources; `NowWatchingControllerTests`)
- Test: `tests/unit/NowWatchingControllerTests.cpp`

**Interfaces:**
- Consumes: `ActiveSessionRegistry`, `EpisodeRecognitionService::activeSessions()` (Task 2), `IMediaReader`, `RecognitionContext::recognition`.
- Produces: `NowWatchingController(const ActiveSessionRegistry &, IMediaReader &, std::function<void(const QString&)> warning = {}, QObject *parent = nullptr)` with `Q_PROPERTY(bool active)`, `Q_PROPERTY(QVariantList sessions)` (items `{key, label}`), `Q_PROPERTY(QString selectedSessionKey)`, `Q_PROPERTY(QVariantMap current)`, `Q_INVOKABLE void SelectSession(const QString&)`, `void Refresh()`, signal `changed()`. `current` keys: `associated`, `animeTitle`, `coverUrl`, `progress` (`"6/12"`, `"6/?"` when total unknown, via existing `FormatProgress`), `listStatus`, `season`, `episode`, `episodeTitle`, `summary`, `duration` (`"24 min"`), `airDate` (`dd/MM/yyyy`), `source` (`mpv`/`Plex`), `user`, `device`, `sourceTitle`. Missing values are **absent keys**, not empty strings.
- Registry callback → `QTimer::singleShot(0, this, &NowWatchingController::Refresh)`.

- [ ] **Step 1: Write the failing tests**: `inactiveWithoutSessions` (`active false`, empty `sessions`, empty `current`); `associatedSessionShowsAnimeFromDatabase` (mediaId 21 → `animeTitle "One Piece"`, `progress "5/12"`, `associated true`, `coverUrl` from `Media::CoverLargeUrl`); `unassociatedSessionShowsEpisodeOnly` (`associated false`, no `animeTitle`, `sourceTitle` present); `omitsMissingEpisodeFields` (no summary/airDate → keys absent); `defaultsToMostRecentSession`; `selectSessionSwitchesCurrent`; `selectionMovesWhenSelectedSessionEnds`; `mediaReadFailureFallsBackToUnassociatedAndWarns` (warning callback called once, no secret in message); `emitsChangedWhenRegistryChanges` (QSignalSpy after `QCoreApplication::processEvents()`).
- [ ] **Step 2: Run** `NowWatchingControllerTests`. Expected: FAIL.
- [ ] **Step 3: Implement** the controller; labels: `"<animeTitle or sourceTitle> · <user> · <source>"`.
- [ ] **Step 4: Wire `main.cpp`** and run a full build of `HaikenAnime`. Expected: build OK.
- [ ] **Step 5: Run** `NowWatchingControllerTests RecognitionCompositionTests`. Expected: PASS.
- [ ] **Step 6: Commit** `feat: expose now watching sessions to qml`.

### Task 6: NowWatchingScreen, Home button and page

**Files:**
- Create: `resources/qml/NowWatchingScreen.qml`
- Modify: `resources/qml/Home.qml` (signal `openNowWatchingRequested()`; `AppButton` "Assistindo agora" placed **after** the "Configurações" button at `Home.qml:113-119`, `visible: nowWatchingController !== null && nowWatchingController.active`)
- Modify: `resources/qml/Main.qml` (page `"nowWatching"`, `onBackRequested: root.page = "home"`)
- Modify: `CMakeLists.txt` (`qt_add_qml_module` `QML_FILES`)
- Test: `tests/unit/QmlRecognitionStructureTests.cpp`

**Interfaces:**
- Consumes: `NowWatchingController` properties (Task 5).
- Produces: `NowWatchingScreen` with `signal backRequested()` and `property var controller: nowWatchingController`.

- [ ] **Step 1: Write the failing tests**: `nowWatchingScreenLoadsWithController` (fake controller with two sessions loads without errors); `nowWatchingScreenShowsEmptyMessage` (source contains `Nada sendo reproduzido no momento`); `homeButtonIsRightOfSettingsAndGuarded` (in `Home.qml`, the index of `qsTr("Assistindo agora")` is greater than the index of `qsTr("Configurações")` and the button block contains `nowWatchingController !== null`); add `NowWatchingScreen.qml` to `RecognitionFiles` so the forbidden-content and native-button checks cover it; `mainHasNowWatchingPage`.
- [ ] **Step 2: Run** `QmlRecognitionStructureTests`. Expected: FAIL.
- [ ] **Step 3: Use the `/frontend-design` skill** to design the card inside the Global Constraints visual pattern: cover left (existing placeholder `qrc:/qt/qml/HaikenAnime/resources/images/cover-placeholder.svg` when no cover), anime block right, episode block below with summary `maximumLineCount: 4`, session selector (`ComboBox` over `sessions`) shown only when `sessions.length > 1`, the `não associado à biblioteca` label when `current.associated === false`.
- [ ] **Step 4: Implement** the three QML changes; every optional row is `visible: current.<key> !== undefined`.
- [ ] **Step 5: Run** `QmlRecognitionStructureTests QmlStructureTests HomeScreenControllerTests` with `QT_QUICK_BACKEND=software`. Expected: PASS.
- [ ] **Step 6: Run** the full build and full CTest; record the pre-existing failures separately. Expected: no new failures.
- [ ] **Step 7: Commit** `feat: add now watching screen`.

### Task 7: Validation

**Files:**
- Modify: `docs/superpowers/validation/2026-10-17-reconhecimento-episodios.md` (section "Assistindo agora")

- [ ] **Step 1:** On Windows, with mpv enabled, play a library episode through "Assistir próximo"; record that the button appears to the right of "Configurações", the card shows the anime and the mpv title/duration, and the button disappears after closing mpv. Label live checks explicitly.
- [ ] **Step 2:** With a Plex test server and two selected users playing, record the selector with two sessions and the unassociated label for an unmatched title; label as unverified if no server is available.
- [ ] **Step 3:** Run `git diff --check` and inspect the diff for secrets or build outputs.
- [ ] **Step 4: Commit** the validation evidence (`git add -f`, `docs/` is gitignored).
