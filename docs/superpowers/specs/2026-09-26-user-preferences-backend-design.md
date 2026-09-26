# User Preferences Backend Design

## Purpose

Implement the first functional slice of the settings screen. This slice persists and applies only preferences already consumed by the application: score scale, cover quality, automatic synchronization, and synchronization interval. Settings for accounts, local-library recognition, filters, downloads, and history remain visible previews and are outside this implementation.

The user edits a draft and explicitly saves the complete set. A successful save must persist the preferences atomically and apply them to the running process without restarting the application.

## Configuration Boundaries

`Settings.json` remains the packaged source for operational configuration such as endpoints, query paths, HTTP timeouts, retry limits, download limits, and log retention. These values are not editable from QML.

The file gains a `userPreferences` object containing first-run defaults:

```json
{
  "userPreferences": {
    "score": {
      "minimum": 0,
      "maximum": 10,
      "step": 1
    },
    "covers": {
      "quality": "medium"
    },
    "sync": {
      "enabled": true,
      "intervalMs": 3600000
    }
  }
}
```

If the database has no user-preferences row, application composition inserts one from these packaged defaults. Once the row exists, database values take precedence. Invalid packaged defaults are reported through controlled initialization diagnostics and fall back to the current safe application defaults; they must not prevent the window from opening.

## Application Model and Contracts

Add a typed `UserPreferences` application model with:

- `double scoreMinimum`
- `double scoreMaximum`
- `double scoreStep`
- `CoverQuality coverQuality`
- `bool synchronizationEnabled`
- `int synchronizationIntervalMs`

Add an `IUserPreferencesRepository` application port with read and replace operations. Replace writes the complete model as one logical operation. The application layer owns validation rules so they are shared by presentation and persistence callers.

Validation requires:

- finite score values;
- minimum strictly lower than maximum;
- step greater than zero and no larger than the score range;
- the range to be divisible by the step within a small floating-point tolerance;
- a supported cover quality;
- a positive synchronization interval within the explicit bounds exposed by the controller.

The initial UI offers practical interval options rather than accepting arbitrary milliseconds. The controller exposes those options as semantic values and labels so QML does not define backend policy.

## SQLite Persistence

Add a typed singleton table:

```sql
CREATE TABLE user_preferences (
    id INTEGER PRIMARY KEY CHECK (id = 1),
    score_minimum REAL NOT NULL,
    score_maximum REAL NOT NULL,
    score_step REAL NOT NULL,
    cover_quality TEXT NOT NULL,
    synchronization_enabled INTEGER NOT NULL CHECK (synchronization_enabled IN (0, 1)),
    synchronization_interval_ms INTEGER NOT NULL
);
```

The schema migration is transactional and records a new schema version only after all additions succeed. Existing databases retain all media, pending changes, and cover-cache data.

`SqliteUserPreferencesRepository` implements the application port. Its replace operation uses an upsert inside a transaction, validates before writing, and returns a controlled error without changing the stored row when any step fails. SQL remains in external resource files and is registered through the existing SQLite query configuration.

## Cover Variant Persistence and Resolution

The current media record retains only one `cover_url`, even though AniList supplies `medium`, `large`, and `extraLarge`. Add nullable columns for all three variants while retaining `cover_url` as the original/effective URL for backward compatibility.

Extend the integration DTO-to-domain mapping, media upsert, media read, and fixtures so synchronization preserves each supplied variant. Existing rows with only `cover_url` remain valid.

A focused cover-source resolver selects the effective URL in this order:

1. the URL for the requested quality;
2. the original `cover_url` already stored for the media;
3. a non-empty alternative variant, preferring the closest quality;
4. no URL, which leaves the placeholder in use.

The fallback order between alternative variants is deterministic:

- requested `medium`: `large`, then `extraLarge`;
- requested `large`: `medium`, then `extraLarge`;
- requested `extraLarge`: `large`, then `medium`.

When the preference changes, currently displayed cover files remain visible. The home controller changes the requested quality and asks the visible and prefetched windows to resolve again. A newly downloaded file replaces the displayed source only after successful publication. The existing coordinator removes the previous file after the replacement cache entry is committed. A failed replacement leaves the current displayed image intact.

For an original URL whose precise quality is unknown, the cache entry records the requested preference as the resolution context. If a later synchronization supplies the requested variant, the changed remote URL causes the normal cache flow to fetch and replace it.

## Presentation Controller

Add a screen-specific `SettingsController`; do not expand `HomeScreenController` into a settings controller. It exposes:

- persisted values;
- editable draft values;
- backend-defined score and synchronization interval options;
- available cover-quality options;
- dirty and validity state;
- save state and user-facing error or success message.

QML may update draft properties, but it does not write to the repository. `Save()` validates the full draft and calls the repository once. Only after the database commit succeeds does the controller replace its persisted snapshot, clear the dirty state, and emit `preferencesApplied(UserPreferences)`.

Changing sections preserves the draft. Returning to the library with a dirty draft opens a confirmation popup with discard and remain actions. Discard restores the last persisted snapshot. Categories outside this slice remain navigable but are explicitly marked unavailable; their controls do not mutate the draft.

The Save button is enabled only for a valid dirty draft while no save is in progress. Field-level validation explains invalid score combinations or interval values. Repository failures appear as a screen-level message, preserve the draft, and apply nothing in memory.

## Runtime Application

Application composition creates the repository, loads or seeds preferences, and creates the settings controller. `main.cpp` remains limited to Qt/QML initialization and signal wiring. It publishes `settingsController` to QML alongside `homeController`.

After a successful save:

- `HomeScreenController` receives the score range and increment through its existing score-scale configuration entry point.
- `HomeScreenController` receives the cover quality through a focused setter, retains current cover sources, and triggers window re-resolution when the library becomes visible.
- `InitialSyncCoordinator` receives enabled state and interval through a new scheduling method.

Automatic synchronization behavior is:

- disabled at startup: do not start initial or periodic automatic synchronization;
- disabled during an active synchronization: allow the active operation to finish, then leave the scheduler stopped;
- enabled while idle: schedule the next execution from the time the preference is applied;
- interval changed while enabled: restart the timer using the new interval from the time of application;
- enabled during an active synchronization: finish the operation, then schedule using the selected interval.

The preference does not add a manual synchronization action in this slice.

Persistence always precedes runtime application. Runtime setters are deterministic, non-failing operations; therefore a database failure cannot leave the process using values that were not committed.

## QML Integration

The existing settings screen becomes controller-backed:

- Account and synchronization contains functional score-scale and synchronization controls.
- Appearance contains the functional cover-quality selector.
- Save Changes performs the single atomic save.
- The header reports dirty, saving, saved, and failed states without blocking navigation between categories.
- The overview and lateral navigation remain functional.
- Future sections are visibly unavailable rather than appearing to save placeholder values.

The QML receives labels, keys, ranges, and interval choices from `SettingsController`. It must not duplicate supported cover qualities, score formats, or synchronization intervals as policy constants.

## Failure Handling

- A missing preference row is seeded from validated packaged defaults.
- An unreadable preference row produces a controlled initialization error and leaves the settings screen available with safe defaults.
- Validation errors do not call the repository.
- Transaction, prepare, or execution failures roll back the write and preserve the draft.
- Cover replacement failures preserve the last displayed and cached valid cover.
- Settings failures must not prevent the application window from opening.
- Relevant failures and successful saves use the existing standardized logging categories and asynchronous logger.

## Tests

Add or update unit tests for:

- migration creation and idempotency for `user_preferences` and cover-variant columns;
- upgrade from the current schema without loss of media or cover-cache data;
- reading, seeding, and atomic replacement in `SqliteUserPreferencesRepository` using temporary databases;
- valid and invalid `UserPreferences` combinations;
- parsing packaged user-preference defaults from `Settings.json`;
- settings-controller initial state, draft changes, dirty state, validation, save success, rollback-visible failure, discard, and emitted application signal;
- score-scale application in `HomeScreenController`;
- synchronization disabled at startup, disabled while active, enabled while idle, and interval rescheduling;
- persistence and round-trip mapping of all cover variants;
- requested-quality, original-URL, nearest-alternative, and placeholder cover-source resolution;
- retaining a displayed cover until a replacement succeeds.

Run the affected unit executables, the complete CTest suite when feasible, QML linting, application build, a short runtime smoke test, and `git diff --check`. A successful build is not by itself visual validation; inspect the settings screen at the supported wide and minimum window sizes before considering the frontend integration complete.

## Out of Scope

- AniList authorization or account management;
- editable endpoints, HTTP retry policy, download concurrency, or log retention;
- local-library paths and media recognition;
- list-filter automation;
- torrent and download configuration;
- persisted appearance theme or font settings;
- history storage and display;
- manual synchronization;
- backend persistence for media-edit fields, which belongs to the separate media-update flow.
