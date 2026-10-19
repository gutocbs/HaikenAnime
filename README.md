# HaikenAnime

HaikenAnime is a Qt desktop application for organizing an anime library, keeping personal lists synchronized with [AniList](https://anilist.co/), and connecting catalog data with files stored locally on the computer.

The application separates presentation, application workflows, domain rules, and infrastructure, and keeps local data available when remote synchronization is unavailable. It also recognizes episodes watched in mpv or Plex and updates their progress.

## What it does

- Browse anime and seasonal catalog data.
- Manage personal list status, score, and progress.
- Synchronize media and pending changes with AniList.
- Keep a local SQLite database for media, preferences, covers, sync state, and local files.
- Scan configured directories and recognize local episodes using [Anitomy](https://github.com/erengy/anitomy).
- Download and cache cover images.
- Open local media with the configured player or file handler.
- Configure language, media directories, AniList access, and application preferences.
- Preserve translations for the supported application languages.

## Technology

- C++20
- Qt 6: Core, Gui, Network, QML, Quick, Quick Controls 2, SQL, and Test
- Qt Quick/QML
- CMake 3.21+
- SQLite
- AniList GraphQL
- [Anitomy](https://github.com/erengy/anitomy), included under [`lib/anitomy`](lib/anitomy)

## Architecture

The source tree is organized around explicit application boundaries:

```text
.
├── main.cpp                # Qt/QML startup
├── src/app/                # Application and presentation composition, shutdown, workflow coordinators
├── src/application/        # Use cases, services, ports, and presentation contracts
├── src/domain/             # Media, AniList and recognition domain types and policies
├── src/infrastructure/     # Qt, SQLite, AniList, filesystem, player, and platform adapters
├── src/presentation/       # Controllers and list models exposed to QML
├── resources/qml/          # User interface and reusable QML components
├── resources/sqlite/       # SQL queries, configuration, and migrations
├── resources/anilist/      # AniList GraphQL queries and mutations
├── tests/unit/             # CTest-backed unit tests
├── tests/fixtures/         # Recorded responses used by the tests
├── translations/           # Qt translation sources
├── lib/anitomy/            # Local file-name parsing library
├── CMakeLists.txt          # Application and test targets
└── SERVICE.md              # Program reference: flows, persistence, development and test patterns
```

Presentation code should remain focused on rendering and interaction. Synchronization, persistence, filtering, ordering, recognition, and player decisions belong to the application, domain, or infrastructure layers. See [`SERVICE.md`](SERVICE.md) for how each flow works and how to extend it.

## Requirements

- CMake 3.21 or newer.
- Qt 6 with the components listed above. The current Windows configuration is tested with Qt 6.8.3.
- A C++20-compatible compiler. On Windows, use a compiler that matches the selected Qt kit, such as the Qt MinGW kit or an MSVC kit.
- A working SQLite driver supplied by Qt.

## Build on Windows

Open a Qt-enabled PowerShell or Developer PowerShell at the repository root. Configure an out-of-source build and point CMake to the Qt installation used by your compiler:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build --config Debug --parallel
```

For an MSVC kit, replace `CMAKE_PREFIX_PATH` with the matching Qt installation and use the corresponding Visual Studio generator if CMake does not select it automatically.

The executable is generated in the selected build directory. The exact output path depends on the generator and configuration.

### Release package

To produce a clean Release build packaged with only the executable and its runtime dependencies (Qt, QML, Qt translations, MSVC runtime and DXC), run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-release.ps1
```

The script uses the MSVC 2022 kit (`C:/Qt/6.8.3/msvc2022_64`) and Ninja, recreates `cmake-build-release` from scratch, and writes the package to `dist/`. Override the defaults with `-QtDir`, `-Ninja`, `-BuildDir` or `-OutputDir`.

## Tests

Configure with testing enabled, build the test targets, and run CTest from the build directory:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

The tests cover, among other areas, media rules, AniList parsing and synchronization, SQLite repositories, local-library scanning and recognition, player recognition, settings, covers, translations, composition, and loading the QML screens.

## Running the application

After building, launch the generated `HaikenAnime` executable. On first run, configure the media directories and AniList account in Settings. Network access is required for remote synchronization and catalog requests; locally stored media and database data remain available independently of a successful sync.

Do not commit build directories, generated binaries, local databases, credentials, or downloaded media.

## AniList

AniList GraphQL queries and mutations are stored as application resources under [`resources`](resources) and consumed by the AniList infrastructure layer. Authentication data is kept through the configured local secret-storage implementation; tokens and callback payloads must never be written to logs.

### Removing the AniList account

Settings > Account offers a remove action for the connected AniList account. Removal cancels any running synchronization, then deletes the stored credentials. The local library and all changes that were not yet synchronized are kept. While an account is connected, a new connection cannot be started.

### Pending changes belong to an account

Every unsent change (progress, score, status, removal) is stamped with the AniList account that was active when it was made. A change is only ever sent to that same account: switching or removing the account never discards it, and it is sent once its owner reconnects.

### Removing media from your list

The media details panel has a remove-from-list action that asks for confirmation. Confirming hides the media from the library immediately and queues the deletion, which is sent to AniList on the next synchronization. Later synchronizations do not bring the media back; editing it again from the catalog adds it back to the list. Removal by the user is tracked separately from media that disappeared from AniList. Media removed in the app stay hidden even if they are re-added on the AniList website; to restore one, re-add it in the app (seasonal catalog or editor).

### Progress display and synchronization status

- Chapters, volumes or episodes whose total is unknown are shown as `?` (for example `12/?`), never as `0`.
- A progress bar reports the current stage plus an item count. It is indeterminate when the total is unknown (the user list) and shows a percentage only when the total is known (pending changes). The bar stays hidden while nothing is happening (for example the periodic pending-changes check with nothing to send, or a synchronization skipped because no account is connected): it only appears while there is real progress or after a failure. It disappears a few seconds after the synchronization succeeds or fails, and immediately when a synchronization is cancelled (for example when the account is removed). Synchronization errors are shown there only, not on unrelated screens.
- The main window starts maximized and the library opens on the in-progress (watching/reading) list.

## Development

When changing a signal, slot, context property, or application contract, update its consumers and the relevant unit tests. Prefer adding tests under [`tests/unit`](tests/unit) and running the focused target before the complete CTest suite. Development patterns are described in [`SERVICE.md`](SERVICE.md); rules for AI assistants are in [`AGENTS.md`](AGENTS.md).
