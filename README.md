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

## Development Notes

- Keep C++/QML contracts stable when changing signals, slots or context properties.
- Preserve the existing separation between UI, application orchestration, domain code and infrastructure.
- Update the corresponding QML consumers when changing data emitted by `MainClass`.
- Validate AniList query and mutation changes against the client code and the external API contract.

See [codex.md](codex.md) for repository-specific guidance for AI-assisted development.

## License

No license file is currently included in this repository. Add a license before distributing the project or accepting external contributions under defined terms.
