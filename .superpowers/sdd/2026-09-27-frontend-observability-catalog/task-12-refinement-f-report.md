# Task 12 — Refinement F report

## Scope delivered

- Added a bounded, fetch-time TTL seasonal page cache keyed by the complete `SeasonalCatalogRequest` identity: year, season, adult policy, page, and page size.
- Reused valid cached pages without calling the datasource, evicted oldest entries at the configured bound, and allowed expired or disabled entries to fall through safely to AniList.
- Coalesced rapid filter changes to the latest request and suppressed duplicate in-flight page fetches while retaining full request-identity stale-response protection.
- Added deterministic public AniList request spacing through injected clock/delay seams in tests and a safe runtime delay.
- Preserved retry by bypassing only the current cached first page, so an explicit retry still reaches the datasource after an earlier success.
- Loaded cache TTL, maximum entry count, and minimum request interval from `Settings.json`, with independent built-in fallbacks for missing, malformed, incomplete, or invalid policy values.
- Wired the configured policy and existing logger through application composition. Cache hits, throttling, and request failures use the existing Sync logging boundary without expanding QML state.

## Deliberately excluded

- No personal-list inclusion, Part 3 flow, unrelated QML changes, or cache persistence was added.
- The two-filter gate, adult policy propagation, duplicate-media merge order, pagination bounds, details presentation, and append spinner remain unchanged.

## Validation

- Focused `JsonSettingsReaderTests`, `SeasonalCatalogTests`, `SeasonalCatalogControllerTests`, and `QmlStructureTests` passed.
- Release application build passed.
- `git diff --check` passed.
- Full CTest ran 35/37 tests successfully; the pre-existing unrelated failures remain `LocalLibraryScanCoordinatorTests` and `LocalLibraryScanCompositionTests` (exit `0xc0000602`).
