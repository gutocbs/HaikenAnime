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

find_package(Qt6 REQUIRED COMPONENTS Core Network Sql Test)

enable_testing()

add_executable(HaikenAnime main.cpp
        src/domain/media/Media.h
        src/domain/media/MediaType.h
        src/domain/media/MediaStatus.h
        src/domain/anilist/AniListCredentials.h
        src/domain/anilist/AniListSyncFilter.h
        src/domain/anilist/AniListGraphQlError.h
        src/domain/anilist/AniListGraphQlResponse.h
        src/domain/anilist/AniListMediaDto.h
        src/domain/anilist/AniListPage.h
        src/domain/anilist/AniListField.h
        src/domain/anilist/AniListMergePolicy.h
        src/domain/anilist/AniListMergeResult.h
        src/domain/anilist/AniListPendingChangeStatus.h
        src/domain/anilist/AniListPendingChange.h
        src/domain/anilist/AniListSettings.h
        src/domain/http/HttpSettings.h
        src/application/configuration/Settings.h
        src/application/configuration/ISettingsReader.h
        src/application/anilist/ISecretStore.h
        src/application/anilist/IAniListAuthProvider.h
        src/application/anilist/IAniListDataSource.h
        src/application/anilist/IMediaRepository.h
        src/application/anilist/AniListSyncService.h
        src/application/anilist/AniListSyncService.cpp
        src/application/anilist/AniListSyncWorker.h
        src/application/anilist/AniListSyncWorker.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
        src/application/anilist/AniListPendingChangeFactory.h
        src/application/anilist/AniListPendingChangeFactory.cpp
        src/application/anilist/IPendingChangeRepository.h
        src/application/anilist/AniListAuthManager.h
        src/application/anilist/AniListAuthManager.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteMediaRepository.h
        src/infrastructure/database/SqliteMediaRepository.cpp
        src/infrastructure/database/SqlQueryStore.h
        src/infrastructure/database/SqlQueryStore.cpp
        src/infrastructure/secrets/FileSecretStore.h
        src/infrastructure/secrets/FileSecretStore.cpp
        src/infrastructure/anilist/HttpFactory.h
        src/infrastructure/anilist/HttpFactory.cpp
        src/infrastructure/anilist/AniListGraphQlClient.h
        src/infrastructure/anilist/AniListGraphQlClient.cpp
        src/infrastructure/anilist/GraphQlQueryStore.h
        src/infrastructure/anilist/GraphQlQueryStore.cpp
        src/infrastructure/anilist/FileAniListDataSource.h
        src/infrastructure/anilist/FileAniListDataSource.cpp
        src/infrastructure/anilist/GraphQlAniListDataSource.h
        src/infrastructure/anilist/GraphQlAniListDataSource.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
        src/infrastructure/configuration/JsonSettingsReader.h
        src/infrastructure/configuration/JsonSettingsReader.cpp
)

target_link_libraries(HaikenAnime
        PRIVATE
        Qt6::Core
        Qt6::Network
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

add_executable(FileSecretStoreTests
        tests/unit/FileSecretStoreTests.cpp
        src/infrastructure/secrets/FileSecretStore.h
        src/infrastructure/secrets/FileSecretStore.cpp
)

target_link_libraries(FileSecretStoreTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME FileSecretStoreTests COMMAND FileSecretStoreTests)

add_executable(AniListAuthManagerTests
        tests/unit/AniListAuthManagerTests.cpp
        src/application/anilist/AniListAuthManager.h
        src/application/anilist/AniListAuthManager.cpp
)

target_link_libraries(AniListAuthManagerTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListAuthManagerTests COMMAND AniListAuthManagerTests)

add_executable(AniListFlowTests
        tests/unit/AniListFlowTests.cpp
        src/application/anilist/AniListSyncService.h
        src/application/anilist/AniListSyncService.cpp
        src/application/anilist/IAniListDataSource.h
        src/application/anilist/IMediaRepository.h
        src/infrastructure/anilist/FileAniListDataSource.h
        src/infrastructure/anilist/FileAniListDataSource.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
)

target_link_libraries(AniListFlowTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListFlowTests COMMAND AniListFlowTests)

add_executable(AniListMergePolicyResolverTests
        tests/unit/AniListMergePolicyResolverTests.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
)

target_link_libraries(AniListMergePolicyResolverTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListMergePolicyResolverTests COMMAND AniListMergePolicyResolverTests)

add_executable(AniListPendingChangeFactoryTests
        tests/unit/AniListPendingChangeFactoryTests.cpp
        src/application/anilist/AniListPendingChangeFactory.h
        src/application/anilist/AniListPendingChangeFactory.cpp
)

target_link_libraries(AniListPendingChangeFactoryTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListPendingChangeFactoryTests COMMAND AniListPendingChangeFactoryTests)

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
    foreach (QT_LIB Core Network Sql Test)
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
