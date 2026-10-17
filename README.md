# Haiken Anime

Haiken Anime is a Qt desktop application for managing anime, manga and novel lists. It synchronizes media data with AniList, tracks watching/reading progress, searches local files for episodes, downloads cover images and provides torrent-related tools.

## Features

- Manage current, completed, paused, dropped and planned lists.
- Support for anime, manga and novels.
- AniList synchronization for lists, progress, scores and user data.
- Local database and configurable media directories.
- Automatic recognition of locally available episodes.
- Cover/avatar downloads.
- Anime title parsing with the bundled Anitomy library.
- Torrent search, listing and download support.
- QML interface with Home, configuration and torrent screens.

## Technology

- C++14
- Qt Quick/QML
- Qt Network, NetworkAuth and Concurrent
- qmake
- AniList GraphQL
- [Anitomy](https://github.com/erengy/anitomy), included in [lib/anitomy](lib/anitomy)
- [robotlib](https://github.com/Robot/robot), included in [lib/robotlib](lib/robotlib)

## Repository Layout

```text
.
├── QML/                 # User interface and reusable QML components
├── qrc/Anilist/         # AniList GraphQL queries and mutations
├── src/base/            # Domain models, database, configuration and workers
├── src/clients/         # External service clients, including AniList
├── src/utilities/       # Networking, downloading, logging and helpers
├── lib/anitomy/         # File-name parser
├── lib/robotlib/        # Windows automation support
├── main.cpp             # Application entry point
├── mainclass.*          # C++ facade exposed to QML
└── HaikenAnime.pro      # qmake project definition
```

The repository includes source copies of these third-party libraries:

- [Anitomy](https://github.com/erengy/anitomy) — anime file-name parsing.
- [robotlib](https://github.com/Robot/robot) — Windows automation support.

Refer to the original repositories for their respective licenses and upstream project information.

## Requirements

Install a Qt development environment with:

- Qt 5 components for Quick, Network, NetworkAuth and Concurrent.
- A C++14-compatible compiler.
- qmake and the Qt build tools.
- On Windows, the platform libraries required by `lib/robotlib` and `psapi`.

The QML files currently use Qt Quick 2.12, Qt Quick Window 2.15 and Qt Quick Controls 2.3 imports, so use a compatible Qt installation.

## Build

Open a Qt-enabled terminal at the repository root and run:

```bash
qmake HaikenAnime.pro
make
```

On Windows with a MinGW kit, use the corresponding Qt `qmake` and run:

```powershell
qmake HaikenAnime.pro
mingw32-make
```

Alternatively, open `HaikenAnime.pro` in Qt Creator, select a compatible desktop kit, configure the project and build it.

## Run

Launch the generated executable after building. On first run, configure the media directories and AniList account settings from the application configuration screen.

The application stores local configuration, database data and downloaded media according to the configured paths and Qt's application settings location.

## AniList

AniList requests are defined as Qt resources under `qrc/Anilist` and are used by the C++ client in `src/clients`. Network availability and valid account authorization are required for synchronization; the local database remains the source for offline display when remote synchronization is unavailable.

### Removing the AniList account

Settings > Account offers a remove action for the connected AniList account. Removal cancels any running synchronization, then deletes the stored credentials. The local library and all changes that were not yet synchronized are kept. While an account is connected, a new connection cannot be started.

### Pending changes belong to an account

Every unsent change (progress, score, status, removal) is stamped with the AniList account that was active when it was made. A change is only ever sent to that same account: switching or removing the account never discards it, and it is sent once its owner reconnects.

### Removing media from your list

The media details panel has a remove-from-list action that asks for confirmation. Confirming hides the media from the library immediately and queues the deletion, which is sent to AniList on the next synchronization. Later synchronizations do not bring the media back; editing it again from the catalog adds it back to the list. Removal by the user is tracked separately from media that disappeared from AniList. Media removed in the app stay hidden even if they are re-added on the AniList website; to restore one, re-add it in the app (seasonal catalog or editor).

### Progress display and synchronization status

- Chapters, volumes or episodes whose total is unknown are shown as `?` (for example `12/?`), never as `0`.
- A progress bar reports the current stage plus an item count. It is indeterminate when the total is unknown (the user list) and shows a percentage only when the total is known (pending changes). It disappears a few seconds after the synchronization succeeds or fails, and immediately when a synchronization is cancelled (for example when the account is removed). Synchronization errors are shown there only, not on unrelated screens.
- The main window starts maximized and the library opens on the in-progress (watching/reading) list.

## Development Notes

- Keep C++/QML contracts stable when changing signals, slots or context properties.
- Preserve the existing separation between UI, application orchestration, domain code and infrastructure.
- Update the corresponding QML consumers when changing data emitted by `MainClass`.
- Validate AniList query and mutation changes against the client code and the external API contract.

See [codex.md](codex.md) for repository-specific guidance for AI-assisted development.

## License

No license file is currently included in this repository. Add a license before distributing the project or accepting external contributions under defined terms.
