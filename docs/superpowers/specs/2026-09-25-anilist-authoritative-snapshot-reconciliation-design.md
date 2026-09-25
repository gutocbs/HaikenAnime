# AniList Authoritative Snapshot Reconciliation Design

## Objective

Reconcile the local library with a complete AniList snapshot without losing local user data. Media absent from a successful complete synchronization is logically removed from the active library, while media that later reappears is reactivated and continues through the existing merge flow.

## Scope

This design covers:

- recognizing when an AniList synchronization is a complete authoritative snapshot;
- logical source removal through `media.source_removed_at`;
- reactivation when a removed media returns;
- filtering removed media from the active library;
- preserving local fields, pending changes, cache metadata, and cover files;
- correcting the recorded GraphQL fixture so its pagination metadata is truthful;
- migrations and automated tests for existing and new databases.

It does not add the future removed-media screen, permanent deletion, cover cleanup for logically removed media, or a user-facing conflict-resolution interface.

## Confirmed Product Behavior

- A media absent from a complete AniList synchronization is considered removed at the source.
- Source removal is logical: the row remains in `media` and receives a non-null `source_removed_at` timestamp.
- Logically removed media does not appear in the active library.
- Local progress, status, scores, pending changes, cover cache metadata, and cover files are retained.
- If the media appears in a later synchronization, its catalog data is updated, `source_removed_at` is cleared, and the record becomes active again.
- Reactivation does not bypass the existing merge flow. Local/remote differences continue through the current pending-change processing rules.
- A partial, failed, timed-out, or non-initial-page synchronization never marks unseen media as removed.
- Permanent deletion and a removed-media screen remain future work.

## Authoritative Snapshot Rules

A synchronization is authoritative only when all of these conditions are true:

1. it started at page `1`;
2. every requested page was fetched and persisted successfully;
3. pagination advanced continuously using the returned `currentPage`;
4. the synchronization naturally reached a response with `hasNextPage: false`;
5. pending-change processing completed successfully;
6. no timeout or invalid-data condition occurred.

`hasNextPage: true` always means another page must be requested. It must never trigger reconciliation by itself, and no adapter may silently reinterpret it as the end of the snapshot.

The recorded fixture represents a complete captured library for startup and integration tests. Its document therefore declares `hasNextPage: false`. `RecordedGraphQlAniListDataSource` parses and honors that value instead of overriding pagination metadata in code.

## Data Model and Migration

The `media` table gains:

```sql
source_removed_at TEXT NULL
```

`NULL` means the media is active. A UTC ISO-8601 timestamp means it was absent from the last authoritative snapshot that reconciled it.

Schema version `3` adds the column to existing databases with an explicit migration and includes it in fresh database creation. Migration remains transactional and idempotent through the `schema_version` table; it must not rely only on `CREATE TABLE IF NOT EXISTS`, because that does not alter an existing `media` table.

The timestamp is infrastructure state and is not added to the `Media` domain object in this increment. Active readers filter it in SQL. A future removed-media repository can expose it through a purpose-specific model without changing the active-library contract.

## Application Boundary

Snapshot reconciliation is a separate capability from page upsert. The application layer introduces an interface whose operation receives the complete set of media IDs observed during the synchronization and marks active rows not in that set as source-removed.

`AniListSyncService` depends on that capability in addition to `IMediaWriter`. It accumulates unique IDs across pages but invokes reconciliation only after the authoritative-snapshot rules are satisfied. Keeping reconciliation separate makes the destructive interpretation of absence explicit and prevents an ordinary page write from being mistaken for a complete library replacement.

The reconciliation call is the final persistence phase after pending-change processing. If fetching, upsert, timeout validation, or merge processing fails, the call is skipped and previously active media remains active.

## Persistence Behavior

The SQLite repository performs reconciliation in one transaction:

- active rows whose IDs were observed remain unchanged;
- active rows whose IDs were not observed receive the same current UTC timestamp;
- rows already marked removed remain unchanged;
- an empty observed-ID set is valid only after a successful authoritative snapshot and marks every active row removed.

The operation uses bound values and bounded SQL statements rather than constructing an unbounded `NOT IN (...)` string. This avoids SQLite parameter limits and keeps behavior predictable for larger libraries.

The media upsert clears `source_removed_at` on both insert and conflict update. Therefore, reappearance atomically updates remote-authoritative catalog fields and reactivates the row while preserving existing local-authoritative fields that are intentionally absent from the update clause.

The active-library query adds `WHERE source_removed_at IS NULL`. No QML or presentation-layer filtering is introduced.

## Covers and Local Data

Logical source removal does not delete the `media` row, so foreign-key relationships remain intact. Cover cache metadata and cover files are retained and can be displayed immediately if the media is reactivated while the file still exists.

The existing cover URL replacement flow still applies after reactivation: if AniList returns a different URL, the new cover is downloaded and validated, then the old file is removed only after successful replacement.

## Error Handling and Logging

Reconciliation failure makes synchronization fail and reports a database-classified error. It does not partially mark rows because the repository operation is transactional.

Logs distinguish:

- completion of an authoritative snapshot and the number of observed media;
- the number of media newly marked as source-removed;
- skipped reconciliation because the synchronization was partial or failed;
- reactivation through upsert when useful for diagnostics.

Logs do not treat already removed rows as newly removed and do not delete or invalidate cover cache entries.

## Testing and Validation

Tests cover:

- migration of a version-2 database to version 3 without losing media or cover data;
- fresh schema creation with nullable `source_removed_at`;
- active readers excluding logically removed media;
- authoritative synchronization marking unseen active media as removed;
- observed media remaining active;
- an authoritative empty snapshot removing all active media;
- `hasNextPage: true` fetching the next page and postponing reconciliation;
- fetch, upsert, timeout, invalid-page, and pending-merge failures skipping reconciliation;
- synchronization starting after page 1 skipping reconciliation;
- reappearance clearing `source_removed_at` while preserving local fields;
- reactivated media still entering pending-change processing;
- recorded fixture declaring `hasNextPage: false` and being consumed without an adapter override;
- existing synthetic fixture rows disappearing from the active library after the next successful recorded authoritative synchronization;
- cover cache rows remaining associated with logically removed media.

Validation includes the focused unit and integration tests, the complete CTest suite, application build, and `git diff --check`.

## Deferred Work

- a removed-media library view;
- permanent deletion initiated by the user;
- retention or cleanup policies for long-removed media;
- synchronization provenance beyond AniList if multiple authoritative sources are introduced;
- a user-facing merge/conflict screen.
