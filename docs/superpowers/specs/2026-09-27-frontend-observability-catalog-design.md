# Frontend, Observability, and Catalog Improvements Design

## Objective

Improve the clarity, persistence, observability, and discoverability of the current HaikenAnime frontend without overloading the home screen. The work is delivered as independent increments, and each increment must be validated before the next begins.

This design also establishes the data and navigation foundations for full media details, configurable presentation, localization, and a separate AniList seasonal catalog.

## Delivery Policy

- Implement one increment at a time in the order defined by the implementation plan.
- Each increment receives focused automated tests, a full regression run, and relevant visual or runtime validation.
- Do not begin the next increment until the current increment has been reviewed and accepted.
- QML receives presentation-ready values and options. Persistence, fallback rules, filtering, sorting, localization selection, and catalog query construction remain outside QML.
- Startup failures in cleanup, logging, translation loading, or optional catalog features must not prevent the main window from opening.

## Scope

### Included

- Responsive scan-extension layout.
- Startup cleanup of abandoned cover-download temporary files.
- Local-library scan lifecycle logging.
- Persistent home ordering and deterministic initial selector labels.
- Clear card labels and configurable card-status meaning.
- Selectable detail text.
- Expanded cover preview using the already available image.
- Application localization with a persisted language preference applied after restart.
- Persisted preferred title language: Romaji, English, or native.
- Extended media metadata needed by a full-details view and seasonal catalog.
- Full, scrollable media-details overlay.
- A dedicated AniList seasonal-catalog screen requiring both year and season.

### Excluded

- Downloading a higher-quality image specifically for expanded preview.
- Local-file title or episode recognition.
- Associating scanned files with AniList media.
- Launching media files or configuring players.
- Offline mirroring of the complete AniList catalog.
- Automatic background loading of a season before both required filters are selected.

## 1. Scan Extension Layout

The Library settings section displays backend-provided extensions in a responsive grid rather than a single vertical list.

- Use four columns when the content panel can preserve readable checkboxes without clipping.
- Fall back to two columns at intermediate widths and one column at narrow widths.
- Column calculation must use the actual content width, not the application-window width.
- Items follow a stable row-major order supplied by the controller.
- Mouse, keyboard focus, screen-reader labels, and the existing persisted selection behavior remain available.

## 2. Temporary Cover Cleanup

At application startup, HaikenAnime removes abandoned files from its own cover-download temporary directory before accepting new cover downloads.

- Cleanup is restricted to the application-owned directory under the operating-system temporary location, currently `HaikenAnime/covers`.
- The persistent cover cache is never included.
- Cleanup must not traverse symbolic links, junctions, or paths outside the configured temporary root.
- A missing directory is a successful no-op.
- Failure to remove one or more files is logged and treated as non-fatal; the application still opens.
- Active-download cleanup is unnecessary because cleanup runs before the downloader begins accepting work.

## 3. Local-Library Scan Logging

The async logger gains a dedicated local-library category. Every scan produces a bounded lifecycle record:

- `Info` at start: normalized root and enabled extension count.
- `Info` at successful completion: duration, visited entries, accepted candidates, skipped files, and persisted changes.
- `Warning` when a successful scan finds zero candidate files.
- `Error` for invalid requests, directory enumeration failures, repository failures, incomplete reconciliation, or unexpected worker termination.
- Interrupted application shutdown is logged distinctly from a scan failure.

Logs must not contain media-file contents, AniList tokens, credentials, or uncontrolled binary data. Repeated per-file success logging is excluded to avoid log volume and disk contention.

## 4. Persistent Browsing Preferences

User preferences gain a persistent home sort key. The saved key is interpreted against the backend-provided sort options.

- The controller restores the saved sort before the first media-model rebuild.
- If the saved key is missing or no longer offered, the first backend-provided option becomes active and may be persisted as the corrected value.
- Search text remains session-only.
- Media type and list filter remain session-only in this increment unless later requirements explicitly make them persistent.
- Saving sort preference follows the existing atomic preference workflow and schema-migration rules.

The List and Sort selectors always display their effective initial values. They must not render an empty label while options and active keys are being configured. QML binds to explicit controller state and does not infer defaults from hard-coded indices.

## 5. Media Card Semantics

Cards retain title, one status line, progress, and score, but every non-title value becomes self-describing.

Default presentation:

- Personal list status: `Minha lista: Concluído`.
- Progress: `Progresso 12/24`.
- Personal score: `Nota 9/10`, using the configured score scale.

When progress is unavailable, the controller returns the localized form `Progresso —`. When no personal score exists, it returns `Nota —`. QML does not invent or reinterpret these placeholders.

User preferences also gain a card-status presentation choice:

- `personal-list-status` is the default and displays the user's AniList list state.
- `media-release-status` displays the work's release status using a label such as `Exibição: Concluído`.

The backend supplies the option keys and localized labels to the settings controller. The selected key is persisted and validated with a safe fallback.

## 6. Selectable Details Text

Informational text in the details area is read-only but selectable.

- Users can select title, synopsis, alternative names, and other textual values with the mouse and copy them with standard keyboard shortcuts.
- Selection must not accidentally activate media cards, close overlays, or enter edit mode.
- Keyboard navigation and accessibility names remain intact.

## 7. Expanded Cover Preview

Clicking the cover in the details panel opens a modal image preview.

- The preview uses the current resolved cover source, including an existing cached file or placeholder.
- Opening the preview never triggers a request for a higher-quality cover.
- The image preserves aspect ratio and is bounded by the available viewport.
- The preview closes through a clearly separated close button, clicking the backdrop, or pressing `Escape`.
- Focus is trapped while open and restored to the cover trigger when closed.

## 8. Localization

HaikenAnime uses Qt translation catalogs (`.ts` sources compiled to `.qm`) rather than runtime JSON strings in QML.

- Initial supported languages are Brazilian Portuguese and English.
- Translatable text in QML uses `qsTr`; translatable controller/domain presentation text uses Qt translation facilities with stable contexts.
- The Appearance settings section receives a backend-provided language selector.
- The selected language is persisted in SQLite through `UserPreferences`.
- A language change is saved immediately with the other settings but becomes effective only after application restart. The UI communicates this requirement.
- On startup, the translator is installed before QML component creation.
- Missing or invalid catalogs fall back to the application's source language and emit a non-fatal configuration warning.

Backend data identifiers, database keys, JSON keys, and option keys remain English and are never translated.

## 9. Preferred Media Title

The Appearance settings section also receives a persisted title preference with keys `romaji`, `english`, and `native`.

The presentation layer resolves the displayed title with deterministic fallback:

1. The configured title variant when non-empty.
2. Romaji when non-empty.
3. English when non-empty.
4. Native when non-empty.
5. The existing canonical name.

The resolved title is used consistently in cards, home details, full-list results, full-details view, seasonal catalog, sorting by title, and title search. Original title variants remain stored independently and visible in full details.

## 10. Extended Media Metadata

The media domain, AniList mapping, SQLite schema, external SQL resources, and fixtures are extended only with fields required by the approved views:

- Romaji, English, and native titles.
- Synopsis in a presentation-safe plain-text form.
- Release status.
- Season and season year.
- Next-airing episode number and timestamp when available.
- AniList page URL.
- External and streaming links represented as structured site-name/URL pairs.

HTML returned in AniList descriptions is normalized outside QML while preserving meaningful paragraph breaks. URLs must use accepted HTTP or HTTPS schemes. Duplicate links are collapsed by normalized site and URL. Missing optional fields do not fail the media record.

Schema changes use forward-only migrations and preserve existing media, user edits, cover cache, and local-file inventory.

## 11. Full Media Details Overlay

The compact details panel keeps its current role. A dedicated action labeled `Ver detalhes`, and activation of the truncated synopsis, opens a larger left-side overlay following the interaction model of the full-list panel.

The overlay contains:

- Current cover and preferred display title.
- Full selectable, scrollable synopsis.
- Season and year.
- Romaji, English, native, and alternative titles.
- Release status and personal list status as separately labeled values.
- Progress and personal score.
- Next episode date and number when available.
- One AniList link.
- Deduplicated streaming or external links, one row per site.

The overlay remains open while text is selected or links are inspected. It closes only through its right-aligned close control, backdrop click outside the panel, or `Escape`. External links open only after explicit activation and use the system URL handler.

## 12. Seasonal AniList Catalog

A new top-level screen presents the AniList seasonal catalog without adding controls to the home screen.

### Query gate

- Year and season are both required.
- Seasons use stable backend keys: `WINTER`, `SPRING`, `SUMMER`, and `FALL`; localized labels may display Winter, Spring, Summer, and Autumn equivalents.
- No catalog network request runs until both values are selected.
- Changing either value invalidates the current result and starts a new query only when both remain valid.

### Catalog behavior

- Results include the entire AniList catalog for the selected year and season, not only media in the user's list or local library.
- Results are paginated through the existing AniList boundary rather than loaded as one unbounded response.
- The controller owns query state, paging, deduplication, retry, and presentation-ready records.
- QML exposes loading, populated, empty, and error states with an explicit retry action.
- Repeated page requests and stale responses from a previous year/season selection are ignored.
- Catalog cards reuse the preferred-title and cover-resolution rules where applicable, but catalog browsing does not automatically add media to the user's list.
- Selecting a catalog result opens the same full-details presentation contract, populated from catalog data.

The seasonal screen does not start an automatic query at application startup and does not silently choose a default year or season. This preserves the explicit two-filter requirement and prevents unexpectedly large requests.

## Data Ownership and Boundaries

- `UserPreferences` owns persisted presentation choices: sort key, language key, preferred title key, and card-status key.
- Settings repositories and migrations own durable preference storage.
- Home and catalog controllers expose validated options and presentation-ready labels.
- Media repositories own durable media metadata.
- AniList clients and mappers own remote field parsing and pagination envelopes.
- QML owns layout and interaction only; it does not choose defaults, translate backend identifiers, parse AniList HTML, construct GraphQL filters, or sort/filter raw records.

## Error Handling

- Invalid persisted option keys fall back deterministically and produce a configuration warning.
- Temporary-file cleanup failures, missing translation files, and seasonal-catalog network failures remain non-fatal.
- Database migration or core repository failures retain the application's existing visible infrastructure-error behavior.
- A stale seasonal response must never replace results for the currently selected year and season.
- External links with unsupported or malformed schemes are omitted and logged at warning level.

## Validation Strategy

Every increment requires:

- Focused unit tests for controller, validation, persistence, or infrastructure behavior.
- QML structural or compilation checks for presentation bindings.
- Full build and complete CTest regression.
- Visual verification at wide and narrow widths for UI changes.
- Database tests for both a fresh schema and migration from the previous version.

Additional acceptance checks include:

- Restart restores sort, language, title preference, and card-status preference.
- Language changes do not affect the current process and do affect the next process.
- Extension layout changes column count without clipping or losing selection.
- Startup removes only abandoned application-owned temporary covers.
- A zero-result scan logs a warning and still completes successfully.
- Long synopsis text is complete and scrollable in the details overlay.
- Title search and sorting use the configured resolved-title rule.
- Seasonal catalog sends no request with only one selector chosen and paginates the complete selected season once both are chosen.

## Deferred Decisions

- Persisting the current media type and personal-list filter.
- Runtime language switching without restart.
- User-configurable card layouts beyond the single status-source preference.
- Seasonal-catalog sorting and filtering beyond the required year and season.
- Adding catalog entries to the user's AniList list.
- Offline seasonal-catalog caching policy.
