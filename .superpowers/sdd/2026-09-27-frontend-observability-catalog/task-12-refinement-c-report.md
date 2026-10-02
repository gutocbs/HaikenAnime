# Task 12 — Refinement C report

## Scope delivered

- Added persisted `includeAdultContent`, with `false` as its default in the preference model and SQLite schema migration 14.
- Preserved existing preferences during save/reload and migration; the setting is exposed through the existing Settings controller and its backend-owned QML switch.
- Propagated the effective policy into `SeasonalCatalogRequest` and the AniList GraphQL variables before the request is made. Disabled requests send `isAdult: false`; enabled requests send `isAdult: null`, leaving adult media eligible without post-response filtering.
- Added the policy to the seasonal coordinator/controller selection state. A policy change resets the active result set, increments the request generation, and reloads the selected year/season so stale responses cannot replace the new policy's result.
- Applied saved settings through the existing `preferencesApplied` flow. The app does not apply an unsaved draft.

## Deliberately excluded

- No cache, cache-key implementation, request coalescing, or rate limiting was added (Refinement F). The request equality now includes the policy so a future cache key can distinguish it.
- No centered seasonal states (Refinement E), personal-list work, or unrelated QML redesign was included.

## Validation

- Focused settings, SQLite migration/repository, seasonal catalog/controller, and QML structure tests passed.
- Release build passed.
- `git diff --check` passed for the committed Refinement C paths.
