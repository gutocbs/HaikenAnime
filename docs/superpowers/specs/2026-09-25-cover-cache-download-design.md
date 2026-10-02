# Cover Cache and Download Design

## Objective

Add persistent, offline-capable media covers without making QML responsible for HTTP, filesystem access, cache policy, or download scheduling. Covers must load incrementally as the user browses the library, remain available between executions, and be replaced safely when AniList changes a cover URL.

## Scope

This design covers:

- AniList cover variants and cover metadata;
- persistent files and SQLite cache metadata;
- lazy download scheduling based on the visible library window;
- controlled concurrency, retry, and prefetch;
- per-item model updates;
- validation, replacement, manual cleanup, and missing-file recovery;
- migration of the current library container to a virtualized `GridView`;
- configuration fields required before a settings screen exists.

It does not add the settings screen, general-purpose image caching, background download of the entire library, or local recompression in the first implementation.

## Confirmed Product Behavior

- A downloaded cover remains visible without internet while its file exists.
- Covers remain on disk while their media exists in the library.
- Removing a media item removes its cache metadata and cover file.
- A manual **Clear images** action removes cached files and cache metadata, but preserves remote cover URLs in the media catalog.
- Clearing images does not redownload the whole library. Covers return lazily when media becomes visible or enters the prefetch window.
- If a cached file is deleted outside the application, it is treated as missing and downloaded again when needed.
- A changed remote URL starts a replacement download. The old image stays visible until the new image is validated and published.
- A failed replacement never deletes the last valid local cover.
- A media item with no usable image displays a packaged placeholder.

## AniList Cover Contract

AniList exposes `medium`, `large`, and `extraLarge` cover URLs. The initial default is `medium`, which is sufficient for the current card size. `large` and `extraLarge` are accepted configuration values for future settings UI.

The catalog stores the selected remote URL as remote-authoritative data. Cache paths are infrastructure state and do not replace that URL in the `Media` domain model or the `media.cover_url` column.

The first implementation downloads the requested AniList variant without local resizing or recompression. This avoids downloading a larger file only to reduce it locally and avoids an additional quality loss. Local transformation can be reconsidered only if measured storage or rendering costs justify it.

## Architecture

```text
Home.qml / GridView
    -> HomeScreenController
        -> CoverDownloadCoordinator
            -> ICoverCacheRepository
            -> ICoverFileStore
            -> ICoverDownloader
```

### Presentation

`Home.qml` renders prepared state and reports the current visible range. It does not create HTTP requests, choose cache paths, inspect files, or apply retry rules.

The current `GridLayout + Repeater` creates every card and cannot provide a meaningful visible window. It will be replaced by a virtualized `GridView` while preserving the current visual composition, two/three-column behavior, card dimensions, and information architecture.

`HomeMediaModel` exposes:

- `remoteCoverUrl`: catalog URL retained for diagnostics and orchestration, not loaded directly by QML;
- `coverSource`: valid local file URL or the packaged placeholder;
- `coverState`: `missing`, `queued`, `downloading`, `available`, or `failed`.

On successful publication, the model updates only the affected row and emits `dataChanged()` for the cover roles. It never resets or reloads the entire model for one cover.

If QML cannot read a previously resolved local file, it reports the media ID to `HomeScreenController`. The controller invalidates that cache entry, restores the placeholder, and asks the coordinator to enqueue the cover again. QML reports the presentation failure but does not decide what to download.

### Application orchestration

`CoverDownloadCoordinator` owns request state, priority, deduplication, retries, and per-item completion. It receives media IDs and remote URLs for the visible and prefetch windows and emits outcomes without depending on QML types.

The visible window has the highest priority. Only the next logical page is prefetched. Requests that have not started can be removed or reprioritized when they leave both windows. A running request may complete rather than being repeatedly aborted during normal scrolling.

Page completion is useful for metrics and prefetch bookkeeping only. It is not a visual barrier: each successful item appears immediately, and one failed item never blocks other covers.

### Infrastructure

`ICoverDownloader` is implemented with Qt Network. Its `QNetworkAccessManager` and replies are created and used in their owning worker thread.

`ICoverFileStore` owns temporary files, image validation, atomic publication, file removal, and manual clearing.

`ICoverCacheRepository` persists cache metadata in SQLite. A concrete SQLite implementation remains in infrastructure and uses the same worker-thread connection ownership rules as other database work.

## Persistent Storage

Cover files live under `QStandardPaths::AppDataLocation`:

```text
HaikenAnime/
|-- haikenanime.sqlite
`-- covers/
    |-- 154587-<url-hash>.jpg
    `-- 116807-<url-hash>.webp
```

The deterministic filename contains the media ID and a cryptographic hash of the selected remote URL. A URL change therefore produces a different destination and cannot overwrite the last valid image prematurely.

The `cover_cache` table records at least:

- `media_id`;
- `remote_url`;
- selected variant;
- local filename or relative path;
- MIME type;
- byte size;
- `ETag`, when supplied;
- `Last-Modified`, when supplied;
- last successful validation time.

The row references `media(id)` with cascade deletion. Paths stored in SQLite are relative to the application data directory so moving the application data root does not invalidate every entry.

Cache metadata is indexed in memory. The application does not decode or stat every cover at startup. Existence is checked lazily for items in the visible or prefetch window and remembered for the session until an error, replacement, or explicit clear invalidates it.

## Download and Replacement Flow

For a missing cover:

1. Resolve the selected variant and remote URL.
2. Deduplicate the request by media ID, URL, and variant.
3. Download to a temporary file in the cover directory.
4. Validate HTTP status, declared and actual size limits, supported MIME, decodability, and dimensions.
5. Atomically publish the validated file at its deterministic destination.
6. Persist the new cache metadata.
7. Notify the controller, which updates only the affected model row.

For a changed URL, the same sequence runs while the old cache entry remains active. After the new row is persisted and published to the model, the old file is removed. A failure before publication leaves the old entry untouched.

An interruption can leave a new file that is not referenced by SQLite. Such files are safe orphans. Maintenance removes them asynchronously in bounded batches or during explicit cleanup; startup is never blocked by a full directory decode or scan.

## Validation Rules

Before publication, `QImageReader` validates that the temporary file is a supported, decodable image. The file store also rejects:

- empty or truncated responses;
- unsupported MIME types;
- payloads above the configured maximum;
- dimensions outside configured safety bounds;
- HTML or JSON error responses disguised as image downloads.

Validation reads the temporary file once. QML performs the later decode needed for display. The cache does not proactively decode every stored image.

## Network Protection

The initial scheduler policy is:

- at most three simultaneous downloads;
- deduplication by media ID, URL, and variant;
- current visible items before prefetch items;
- prefetch limited to one next page;
- at most two retries for timeout, HTTP `408`, `429`, and `5xx`;
- honor `Retry-After` when present;
- bounded backoff between attempts;
- no automatic retry for `404`, invalid MIME, oversize payload, or corrupt image;
- a failure cooldown so scrolling does not immediately repeat a failed request;
- cancellation or removal of queued, not-yet-started work outside the active windows.

The list, database synchronization, and UI never wait synchronously for image downloads.

## Configuration

Settings gain an image/cover section with validated defaults for:

- variant: `medium`, `large`, or `extraLarge`;
- maximum simultaneous downloads, default `3`;
- request timeout;
- maximum retries, default `2`;
- retry delay/backoff inputs;
- maximum response bytes;
- minimum and maximum accepted dimensions;
- failure cooldown.

These fields are configured through `Settings.json` initially and can later be exposed by the settings screen without changing the downloader contracts.

Changing quality does not eagerly redownload the entire cache. Existing valid covers continue to display, and the new variant is acquired lazily as entries become visible. Once a replacement succeeds, the previous variant file is removed.

## Manual Cleanup

The future **Clear images** command is an application operation, not a direct QML filesystem action. It:

1. pauses admission of new cover work;
2. increments a cache generation and cancels queued and running work;
3. removes cache metadata and files;
4. invalidates the in-memory index;
5. updates affected model rows to the placeholder;
6. resumes lazy admission.

Every request captures the generation in which it started. A late completion from an older generation is discarded and cannot recreate cache metadata after cleanup; its temporary file is removed. This prevents a download/clear race without holding the UI thread.

Remote URLs remain in media catalog records. A later visible-window request may download them again. Permanently suppressing covers would be a separate future setting and is outside this design.

## Error Handling

Errors are isolated per media. The coordinator reports structured outcomes that distinguish transport failure, HTTP rejection, invalid content, filesystem failure, and cache persistence failure.

Expected behavior:

- no prior local cover: keep the placeholder;
- valid prior cover: keep displaying it;
- failed media: do not block the rest of the page;
- infrastructure error: log with the appropriate category without exposing URLs containing sensitive query data;
- repeated temporary failure: apply cooldown and allow a later visible request to retry.

## Testing and Validation

Unit and focused integration tests cover:

- cache hit with an existing file;
- metadata row whose file was removed externally;
- valid download and atomic publication;
- empty, truncated, oversized, corrupt, or wrong-MIME responses;
- URL change retaining the old cover until successful replacement;
- failed replacement preserving the old cover;
- removal of the old file after successful replacement;
- request deduplication;
- maximum concurrency of three;
- visible-item priority and one-page prefetch;
- retry classification, `429`, and `Retry-After`;
- failure cooldown;
- manual cleanup preserving remote URLs;
- media deletion cascading to cache cleanup;
- per-row `dataChanged()` without model reset;
- QML read failure invalidating and re-enqueueing one media item;
- settings defaults and invalid values;
- thread ownership and clean shutdown during network/file work.

Tests use temporary directories, temporary SQLite databases, controlled network replies, and generated small image fixtures. They never write to the user's real cache.

After automated validation, the library is inspected at wide and narrow window sizes to confirm that the `GridView` preserves the current layout, scrolling, placeholder, asynchronous replacement, and selection behavior.

## Delivery Boundaries

Implementation proceeds in independently testable increments:

1. settings and cache metadata contract;
2. file store and validation;
3. controlled downloader and scheduler;
4. coordinator and cache replacement;
5. model roles and per-item updates;
6. virtualized QML list and visible-window reporting;
7. manual cleanup operation;
8. full build, CTest, shutdown, and visual validation.

No increment may move HTTP or filesystem policy into QML, block the GUI thread, delete the last valid cover before replacement succeeds, or claim visual correctness based only on compilation.
