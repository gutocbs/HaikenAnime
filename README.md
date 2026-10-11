# HaikenAnime V2

HaikenAnime is a Qt desktop application for organizing an anime library, keeping personal lists synchronized with [AniList](https://anilist.co/), and connecting catalog data with files stored locally on the computer.

V2 is the current application architecture: it separates presentation, application workflows, domain rules, and infrastructure while keeping local data available when remote synchronization is unavailable.

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
├── QML/                    # User interface and reusable QML components
├── src/app/                # Application composition and workflow coordination
├── src/application/        # Use cases, services, ports, and presentation contracts
├── src/domain/             # Media and AniList domain types and policies
├── src/infrastructure/     # Qt, SQLite, AniList, filesystem, and platform adapters
├── resources/sqlite/       # SQL queries, configuration, and migrations
├── tests/unit/             # CTest-backed unit tests
├── translations/           # Qt translation sources
├── lib/anitomy/            # Local file-name parsing library
├── CMakeLists.txt          # Application and test targets
└── codex.md                # Repository guidance for AI-assisted development
```

Presentation code should remain focused on rendering and interaction. Synchronization, persistence, filtering, ordering, recognition, and player decisions belong to the application, domain, or infrastructure layers.

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

## Tests

Configure with testing enabled, build the test targets, and run CTest from the build directory:

```powershell
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/mingw_64"
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

The tests cover, among other areas, media rules, AniList parsing and synchronization, SQLite repositories, local-library scanning and recognition, settings, covers, translations, and QML structure.

## Running the application

After building, launch the generated `HaikenAnime` executable. On first run, configure the media directories and AniList account in Settings. Network access is required for remote synchronization and catalog requests; locally stored media and database data remain available independently of a successful sync.

Do not commit build directories, generated binaries, local databases, credentials, or downloaded media.

## AniList

AniList GraphQL queries and mutations are stored as application resources under [`resources`](resources) and consumed by the AniList infrastructure layer. Authentication data is kept through the configured local secret-storage implementation; tokens and callback payloads must never be written to logs.

## Development

When changing a signal, slot, context property, or application contract, update its consumers and the relevant unit tests. Prefer adding tests under [`tests/unit`](tests/unit) and running the focused target before the complete CTest suite.
