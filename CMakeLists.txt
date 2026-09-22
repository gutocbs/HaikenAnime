cmake_minimum_required(VERSION 3.21)

project(HaikenAnime LANGUAGES CXX)

if (MINGW)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/mingw_64" CACHE PATH "Qt installation" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql Test)

enable_testing()

add_executable(HaikenAnime main.cpp
        src/domain/media/Media.h
        src/domain/media/MediaType.h
        src/domain/media/MediaStatus.h
        src/domain/anilist/AniListCredentials.h
        src/domain/anilist/AniListPage.h
        src/application/anilist/ISecretStore.h
        src/application/anilist/IAniListAuthProvider.h
        src/application/anilist/IAniListDataSource.h
        src/application/anilist/IMediaRepository.h
        src/application/anilist/AniListSyncService.h
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
)

target_link_libraries(HaikenAnime
        PRIVATE
        Qt6::Core
        Qt6::Sql
)

add_executable(SqliteDatabaseTests
        tests/unit/SqliteDatabaseTests.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
)

target_link_libraries(SqliteDatabaseTests
        PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
)

add_test(NAME SqliteDatabaseTests COMMAND SqliteDatabaseTests)

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    if (MINGW)
        set(QT_INSTALL_PATH "C:/Qt/6.8.3/mingw_64")
    else ()
        set(QT_INSTALL_PATH "${CMAKE_PREFIX_PATH}")
    endif ()
    if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
        set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        if (NOT EXISTS "${QT_INSTALL_PATH}/bin")
            set(QT_INSTALL_PATH "${QT_INSTALL_PATH}/..")
        endif ()
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET HaikenAnime POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:HaikenAnime>/plugins/platforms/")
        add_custom_command(TARGET HaikenAnime POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/platforms/qwindows${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/platforms/")
    endif ()
    if (EXISTS "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll")
        add_custom_command(TARGET HaikenAnime POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:HaikenAnime>/plugins/sqldrivers/")
        add_custom_command(TARGET HaikenAnime POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/plugins/sqldrivers/qsqlite${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/sqldrivers/")
    endif ()
    foreach (QT_LIB Core Sql)
        add_custom_command(TARGET HaikenAnime POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${QT_INSTALL_PATH}/bin/Qt6${QT_LIB}${DEBUG_SUFFIX}.dll"
                "$<TARGET_FILE_DIR:HaikenAnime>")
    endforeach (QT_LIB)

    if (MINGW)
        get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        foreach (MINGW_RUNTIME libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
            add_custom_command(TARGET HaikenAnime POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${MINGW_BIN_DIR}/${MINGW_RUNTIME}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach ()
    endif ()
endif ()
