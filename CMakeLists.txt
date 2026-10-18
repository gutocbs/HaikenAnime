cmake_minimum_required(VERSION 3.21)

project(HaikenAnime LANGUAGES CXX)

if (MINGW AND (NOT CMAKE_PREFIX_PATH OR CMAKE_PREFIX_PATH MATCHES "/msvc"))
    set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/mingw_64" CACHE PATH
            "Qt installation compatible with the MinGW compiler" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Help Qt's WrapVulkanHeaders package find a standard Windows Vulkan SDK
# installation even when the SDK installer has not exported VULKAN_SDK to the
# current terminal process.
if (WIN32 AND NOT DEFINED ENV{VULKAN_SDK})
    file(GLOB VULKAN_SDK_CANDIDATES "C:/VulkanSDK/*")
    if (VULKAN_SDK_CANDIDATES)
        list(SORT VULKAN_SDK_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
        list(GET VULKAN_SDK_CANDIDATES 0 VULKAN_SDK_ROOT)
        set(ENV{VULKAN_SDK} "${VULKAN_SDK_ROOT}")
    endif ()
endif ()
if (DEFINED ENV{VULKAN_SDK} AND EXISTS "$ENV{VULKAN_SDK}/Include/vulkan/vulkan.h")
    set(Vulkan_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include" CACHE PATH
            "Vulkan SDK include directory")
endif ()

find_package(Qt6 REQUIRED COMPONENTS Core Gui LinguistTools Network Qml Quick QuickControls2 Sql)

include(CTest)
if (BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif ()

if (COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif ()

# Third-party file-name parser used by local library recognition.
file(GLOB ANITOMY_SOURCES CONFIGURE_DEPENDS lib/anitomy/*.cpp lib/anitomy/*.h)
add_library(anitomy STATIC ${ANITOMY_SOURCES})

# Every application source except main.cpp. The application and the tests link this library, so each
# source is compiled once. Qt modules stay PRIVATE: each test declares the modules it needs, which also
# decides whether QTEST_MAIN creates a QCoreApplication or a QGuiApplication.
file(GLOB_RECURSE HAIKEN_CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_library(haiken_core STATIC ${HAIKEN_CORE_SOURCES})
target_link_libraries(haiken_core
        PRIVATE
        anitomy
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
)
if (WIN32)
    target_link_libraries(haiken_core PRIVATE Advapi32)
endif ()

qt_add_executable(HaikenAnime WIN32 main.cpp)

file(GLOB SQLITE_MIGRATION_RESOURCES CONFIGURE_DEPENDS resources/sqlite/migrations/*.sql)
foreach (MIGRATION_FILE IN LISTS SQLITE_MIGRATION_RESOURCES)
    get_filename_component(MIGRATION_ALIAS "${MIGRATION_FILE}" NAME)
    set_source_files_properties("${MIGRATION_FILE}" PROPERTIES QT_RESOURCE_ALIAS "${MIGRATION_ALIAS}")
endforeach ()

set(LIBRARY_SCAN_SQL_RESOURCES
        resources/sqlite/queries/begin-library-scan.sql
        resources/sqlite/queries/upsert-local-file.sql
        resources/sqlite/queries/complete-library-scan.sql
        resources/sqlite/queries/fail-library-scan.sql
        resources/sqlite/queries/mark-local-files-unavailable.sql
        resources/sqlite/queries/read-pending-local-files.sql
        resources/sqlite/queries/read-catalog-media-for-recognition.sql
        resources/sqlite/queries/save-local-file-recognition.sql
        resources/sqlite/queries/read-local-files-under-folder.sql
        resources/sqlite/queries/read-local-file-roots.sql
        resources/sqlite/queries/read-next-local-episode.sql
        resources/sqlite/queries/read-available-episode-count.sql
)
set(PLUGIN_SETTINGS_SQL_RESOURCES
        resources/sqlite/queries/read-plugin-settings.sql
        resources/sqlite/queries/upsert-plugin-settings.sql
        resources/sqlite/queries/delete-plugin-settings.sql
)
set(RECOGNITION_HISTORY_SQL_RESOURCES
        resources/sqlite/queries/read-recognition-history.sql
        resources/sqlite/queries/insert-recognition-record.sql
        resources/sqlite/queries/update-recognition-record.sql
        resources/sqlite/queries/delete-recognition-record.sql
        resources/sqlite/queries/discard-recognition-progress.sql
        resources/sqlite/queries/find-queued-recognition-progress.sql
        resources/sqlite/queries/read-media-for-recognition-progress.sql
)
# Every SQL query and GraphQL document is exposed by its file name, e.g. :/sqlite/queries/read-media.sql.
file(GLOB SQLITE_QUERY_RESOURCES CONFIGURE_DEPENDS resources/sqlite/queries/*.sql)
file(GLOB ANILIST_QUERY_RESOURCES CONFIGURE_DEPENDS resources/anilist/queries/*.graphql)
file(GLOB ANILIST_MUTATION_RESOURCES CONFIGURE_DEPENDS resources/anilist/mutations/*.graphql)
foreach (RESOURCE_FILE IN LISTS SQLITE_QUERY_RESOURCES ANILIST_QUERY_RESOURCES ANILIST_MUTATION_RESOURCES)
    get_filename_component(RESOURCE_ALIAS "${RESOURCE_FILE}" NAME)
    set_source_files_properties("${RESOURCE_FILE}" PROPERTIES QT_RESOURCE_ALIAS "${RESOURCE_ALIAS}")
endforeach ()
set_source_files_properties(resources/sqlite/sqlite-queries.json PROPERTIES QT_RESOURCE_ALIAS "sqlite-queries.json")

set_source_files_properties(
        Settings.json
        PROPERTIES QT_RESOURCE_ALIAS "Settings.json")
set_source_files_properties(
        tests/fixtures/media-library.json
        PROPERTIES QT_RESOURCE_ALIAS "media-library.json")

qt_add_resources(HaikenAnime sqlite_resource_bundle
        PREFIX "/sqlite/queries"
        FILES resources/sqlite/sqlite-queries.json ${SQLITE_QUERY_RESOURCES}
)

qt_add_resources(HaikenAnime sqlite_migration_resource_bundle
        PREFIX "/sqlite/migrations"
        FILES ${SQLITE_MIGRATION_RESOURCES}
)

qt_add_resources(HaikenAnime application_config_resource
        PREFIX "/config"
        FILES
        Settings.json
)

qt_add_resources(HaikenAnime anilist_mutation_resource_bundle
        PREFIX "/anilist/mutations"
        FILES ${ANILIST_MUTATION_RESOURCES}
)

qt_add_resources(HaikenAnime anilist_query_resource_bundle
        PREFIX "/anilist/queries"
        FILES ${ANILIST_QUERY_RESOURCES}
)

qt_add_qml_module(HaikenAnime
        URI HaikenAnime
        VERSION 1.0
        QML_FILES
        resources/qml/Main.qml
        resources/qml/Home.qml
        resources/qml/CoverPreview.qml
        resources/qml/MediaDetailsPanel.qml
        resources/qml/SettingsScreen.qml
        resources/qml/SeasonalCatalogScreen.qml
        resources/qml/BrowseControls.qml
        resources/qml/EditMediaPanel.qml
        resources/qml/MediaCard.qml
        resources/qml/StatePanel.qml
        resources/qml/AppButton.qml
        resources/qml/AppTextField.qml
        resources/qml/AppSpinBox.qml
        resources/qml/AppComboBox.qml
        resources/qml/SyncProgressBar.qml
        resources/qml/PluginSettingsScreen.qml
        resources/qml/PluginSettingField.qml
        resources/qml/EpisodeRecognitionScreen.qml
        resources/qml/NowWatchingScreen.qml
        RESOURCES resources/images/cover-placeholder.svg
)

target_link_libraries(HaikenAnime
        PRIVATE
        haiken_core
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
)
if (WIN32)
    target_sources(HaikenAnime PRIVATE resources/icons/HaikenAnime.rc)
endif()

qt_add_resources(HaikenAnime application_icon_resource
        PREFIX "/icons"
        BASE resources/icons
        FILES
        resources/icons/HaikenAnime.png
)

set(HAIKENANIME_TS_FILES
        translations/HaikenAnime_pt_BR.ts
        translations/HaikenAnime_en.ts
)
qt_add_translations(HaikenAnime
        TS_FILES ${HAIKENANIME_TS_FILES}
        RESOURCE_PREFIX "/i18n"
        LUPDATE_OPTIONS -no-obsolete
)

if (BUILD_TESTING)
# Unit tests are separate executables and do not receive the deployment step used by the application
# target, so every test gets the Qt runtime directory on PATH.
get_filename_component(MINGW_RUNTIME_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)

# haiken_add_test(<Name> [QT <module>...] [MIGRATIONS] [SOURCE_DIR] [NO_CORE]
#                 [SOURCES <file>...] [DEFINITIONS <definition>...] [INCLUDES <directory>...])
# Builds tests/unit/<Name>.cpp into its own executable and registers it with CTest.
#   QT           Qt modules besides Core and Test.
#   MIGRATIONS   embeds resources/sqlite/migrations under :/sqlite/migrations.
#   SOURCE_DIR   defines HAIKENANIME_TEST_SOURCE_DIR as the repository root.
#   NO_CORE      does not link haiken_core (the test compiles its own SOURCES).
function(haiken_add_test name)
    cmake_parse_arguments(TEST "MIGRATIONS;SOURCE_DIR;NO_CORE" "" "QT;SOURCES;DEFINITIONS;INCLUDES" ${ARGN})
    add_executable(${name} tests/unit/${name}.cpp ${TEST_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Core Qt6::Test)
    if (NOT TEST_NO_CORE)
        target_link_libraries(${name} PRIVATE haiken_core)
    endif ()
    foreach (MODULE IN LISTS TEST_QT)
        target_link_libraries(${name} PRIVATE Qt6::${MODULE})
    endforeach ()
    if (TEST_SOURCE_DIR)
        target_compile_definitions(${name} PRIVATE HAIKENANIME_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    endif ()
    if (TEST_DEFINITIONS)
        target_compile_definitions(${name} PRIVATE ${TEST_DEFINITIONS})
    endif ()
    if (TEST_INCLUDES)
        target_include_directories(${name} PRIVATE ${TEST_INCLUDES})
    endif ()
    if (TEST_MIGRATIONS)
        qt_add_resources(${name} sqlite_migration_test_resources
                PREFIX "/sqlite/migrations"
                FILES ${SQLITE_MIGRATION_RESOURCES})
    endif ()
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;${MINGW_RUNTIME_DIRECTORY}\\;$ENV{PATH}"
            TIMEOUT 30)
endfunction()

haiken_add_test(MediaDomainTests)
haiken_add_test(RecognitionDomainTests)
haiken_add_test(AlternativeNamesTests)
haiken_add_test(MediaTitleResolverTests)
haiken_add_test(SqliteDatabaseTests QT Sql MIGRATIONS)
haiken_add_test(SqliteUserPreferencesRepositoryTests QT Sql MIGRATIONS SOURCE_DIR)
qt_add_resources(SqliteUserPreferencesRepositoryTests user_preferences_query_resources PREFIX "/sqlite/queries" FILES resources/sqlite/queries/read-user-preferences.sql resources/sqlite/queries/upsert-user-preferences.sql)
haiken_add_test(SqliteRepositoryTests QT Sql MIGRATIONS)
qt_add_resources(SqliteRepositoryTests repository_query_resources PREFIX "/sqlite/queries" FILES resources/sqlite/queries/upsert-media.sql resources/sqlite/queries/read-media.sql resources/sqlite/queries/read-active-media-ids.sql resources/sqlite/queries/mark-media-source-removed.sql resources/sqlite/queries/mark-media-personal-list-removed.sql resources/sqlite/queries/supersede-pending-deletion.sql resources/sqlite/queries/update-personal-list-media.sql resources/sqlite/queries/enqueue-pending-change.sql resources/sqlite/queries/read-pending-changes.sql resources/sqlite/queries/read-pending-media-ids.sql resources/sqlite/queries/read-unsent-media-ids.sql resources/sqlite/queries/update-pending-change.sql resources/sqlite/queries/adopt-orphan-pending-changes.sql)
haiken_add_test(SqliteCoverCacheRepositoryTests QT Sql MIGRATIONS)
qt_add_resources(SqliteCoverCacheRepositoryTests cover_cache_query_resources PREFIX "/sqlite/queries" FILES resources/sqlite/queries/read-cover-cache.sql resources/sqlite/queries/upsert-cover-cache.sql resources/sqlite/queries/delete-cover-cache.sql resources/sqlite/queries/clear-cover-cache.sql)
haiken_add_test(SqliteSyncTaskStateRepositoryTests QT Sql MIGRATIONS)
qt_add_resources(SqliteSyncTaskStateRepositoryTests sync_task_state_resources PREFIX "/sqlite/queries" FILES resources/sqlite/queries/read-sync-task-states.sql resources/sqlite/queries/upsert-sync-task-state.sql resources/sqlite/queries/delete-sync-task-state.sql)
haiken_add_test(SqlitePluginSettingsRepositoryTests QT Sql MIGRATIONS)
qt_add_resources(SqlitePluginSettingsRepositoryTests plugin_settings_query_resources PREFIX "/sqlite/queries" FILES ${PLUGIN_SETTINGS_SQL_RESOURCES})
haiken_add_test(SqliteRecognitionHistoryRepositoryTests QT Sql MIGRATIONS)
qt_add_resources(SqliteRecognitionHistoryRepositoryTests recognition_history_query_resources PREFIX "/sqlite/queries" FILES ${RECOGNITION_HISTORY_SQL_RESOURCES})
haiken_add_test(SqliteMediaProgressReaderTests QT Sql MIGRATIONS)
qt_add_resources(SqliteMediaProgressReaderTests media_progress_reader_query_resources PREFIX "/sqlite/queries" FILES ${RECOGNITION_HISTORY_SQL_RESOURCES})
haiken_add_test(PluginSettingsServiceTests)
haiken_add_test(PlaybackObservationServiceTests)
haiken_add_test(ProgressDecisionServiceTests)
haiken_add_test(RecognitionHistoryServiceTests)
haiken_add_test(RecognitionSyncFeedbackTests)
haiken_add_test(MpvIpcClientTests QT Network SOURCE_DIR)
haiken_add_test(MpvPluginTests)
haiken_add_test(PlexAuthenticationTests QT Network SOURCE_DIR)
haiken_add_test(PlexApiClientTests QT Network SOURCE_DIR)
haiken_add_test(PlexPluginTests QT Network SOURCE_DIR)
haiken_add_test(PluginRegistryTests)
haiken_add_test(ActiveSessionRegistryTests)
haiken_add_test(EpisodeRecognitionServiceTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
haiken_add_test(RecognitionCompositionTests QT Gui Network Sql MIGRATIONS SOURCE_DIR INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
qt_add_resources(RecognitionCompositionTests recognition_composition_query_resources PREFIX "/sqlite/queries" FILES ${PLUGIN_SETTINGS_SQL_RESOURCES} ${RECOGNITION_HISTORY_SQL_RESOURCES})
haiken_add_test(PluginSettingsControllerTests QT Network SOURCE_DIR)
haiken_add_test(EpisodeRecognitionControllerTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
haiken_add_test(NowWatchingControllerTests)
haiken_add_test(SqliteLocalFileRepositoryTests QT Sql MIGRATIONS)
qt_add_resources(SqliteLocalFileRepositoryTests library_scan_test_resources PREFIX "/sqlite/queries" FILES ${LIBRARY_SCAN_SQL_RESOURCES})
haiken_add_test(LocalLibraryScannerTests)
haiken_add_test(AnitomyLocalFileRecognizerTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
haiken_add_test(LocalMediaResolverTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
haiken_add_test(SqlQueryStoreTests)
haiken_add_test(SqliteQueryConfigurationTests)
qt_add_resources(SqliteQueryConfigurationTests sqlite_query_configuration_test_resources PREFIX "/sqlite/queries" FILES resources/sqlite/sqlite-queries.json resources/sqlite/queries/update-personal-list-media.sql ${LIBRARY_SCAN_SQL_RESOURCES})
haiken_add_test(FileSecretStoreTests)
haiken_add_test(JsonSettingsReaderTests SOURCE_DIR)
haiken_add_test(SyncTaskPolicyTests)
haiken_add_test(UserPreferencesValidatorTests)
haiken_add_test(TranslationLoaderTests QT Qml)
haiken_add_test(CoverSettingsTests)
haiken_add_test(CoverSourceResolverTests)
haiken_add_test(CoverFileStoreTests QT Gui INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
haiken_add_test(CoverTemporaryStoreTests DEFINITIONS HAIKENANIME_COVER_TEMPORARY_STORE_TESTING INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src NO_CORE SOURCES src/infrastructure/covers/CoverTemporaryStore.cpp)
haiken_add_test(CoverDownloadCoordinatorTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
haiken_add_test(ClearLocalCacheUseCaseTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
haiken_add_test(QtCoverDownloaderTests QT Network INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
haiken_add_test(AniListAuthManagerTests)
haiken_add_test(AniListOAuthCallbackParserTests)
haiken_add_test(WindowsAniListOAuthLauncherTests QT Gui)
haiken_add_test(AniListOAuthCallbackReceiverTests QT Network)
haiken_add_test(WindowsUrlProtocolRegistrarTests)
haiken_add_test(AniListFlowTests QT Gui DEFINITIONS HAIKENANIME_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
qt_add_resources(AniListFlowTests fixture_test_resource_bundle PREFIX "/fixtures" FILES tests/fixtures/media-library.json)
haiken_add_test(AniListGraphQlParsingTests QT Gui Network DEFINITIONS HAIKENANIME_GRAPHQL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/graphql/page-response.json" HAIKENANIME_GRAPHQL_USERLIST_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/graphql/userlist-response.json")
qt_add_resources(AniListGraphQlParsingTests anilist_partition_query_resource_bundle PREFIX "/anilist/queries" FILES resources/anilist/queries/media-page.graphql resources/anilist/queries/user-media-list.graphql)
haiken_add_test(AniListGraphQlUserListParserTests QT Gui DEFINITIONS HAIKENANIME_USERLIST_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/graphql/userlist-response.json")
haiken_add_test(LocalEpisodeReaderTests QT Sql MIGRATIONS)
qt_add_resources(LocalEpisodeReaderTests local_episode_reader_query_resources PREFIX "/sqlite/queries" FILES resources/sqlite/queries/read-next-local-episode.sql resources/sqlite/queries/read-available-episode-count.sql)
haiken_add_test(QtLocalFileOpenerTests QT Gui)
haiken_add_test(GraphQlQueryStoreTests)
haiken_add_test(SeasonalCatalogTests QT Gui Network DEFINITIONS HAIKENANIME_SEASONAL_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/graphql" INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
qt_add_resources(SeasonalCatalogTests seasonal_catalog_query_resource_bundle PREFIX "/anilist/queries" FILES resources/anilist/queries/seasonal-catalog.graphql)
haiken_add_test(SeasonalCatalogControllerTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src)
haiken_add_test(AniListSyncErrorClassifierTests)
haiken_add_test(AniListMergePolicyResolverTests)
haiken_add_test(AniListPendingChangeFactoryTests)
haiken_add_test(AniListMergeServiceTests)
haiken_add_test(AniListPendingChangeReconcilerTests)
haiken_add_test(AniListPendingChangeProcessorTests)
haiken_add_test(AniListPendingChangeDrainTests)
haiken_add_test(AniListUpdateClientTests QT Network)
haiken_add_test(HomeScreenControllerTests QT Sql SOURCE_DIR)
haiken_add_test(PersonalListChangeServiceTests)
haiken_add_test(AniListAccountOwnershipTests)
haiken_add_test(RemoveAniListAccountUseCaseTests)
haiken_add_test(SettingsControllerTests)
haiken_add_test(SynchronizationStatusControllerTests)
haiken_add_test(QmlStructureTests QT Gui Qml Quick QuickControls2 SOURCE_DIR)
haiken_add_test(QmlRecognitionStructureTests QT Gui Qml Quick QuickControls2 SOURCE_DIR)
haiken_add_test(InitialSyncCoordinatorTests QT Gui Sql MIGRATIONS SOURCE_DIR)
haiken_add_test(AdaptiveSyncCoordinatorTests MIGRATIONS)
haiken_add_test(AdaptiveSyncRuntimeTests)
haiken_add_test(LocalLibraryScanCoordinatorTests)
haiken_add_test(LocalLibraryRecognitionCoordinatorTests INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
haiken_add_test(LocalLibraryScanCompositionTests QT Gui Network Qml Quick QuickControls2 Sql MIGRATIONS)
qt_add_resources(LocalLibraryScanCompositionTests composition_sql_resources PREFIX "/sqlite/queries" FILES resources/sqlite/sqlite-queries.json ${SQLITE_QUERY_RESOURCES})
qt_add_resources(LocalLibraryScanCompositionTests composition_config_resources PREFIX "/config" FILES Settings.json)
haiken_add_test(LocalLibraryRecognitionCompositionTests QT Gui Network Qml Quick QuickControls2 Sql MIGRATIONS)
qt_add_resources(LocalLibraryRecognitionCompositionTests recognition_composition_sql_resources PREFIX "/sqlite/queries" FILES resources/sqlite/sqlite-queries.json ${SQLITE_QUERY_RESOURCES})
qt_add_resources(LocalLibraryRecognitionCompositionTests recognition_composition_config_resources PREFIX "/config" FILES Settings.json)
if (WIN32)
    haiken_add_test(WindowsCredentialStoreTests)
    haiken_add_test(WindowsPluginSecretStoreTests)
endif ()

set_tests_properties(QmlStructureTests QmlRecognitionStructureTests PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>\\;${MINGW_RUNTIME_DIRECTORY}\\;$ENV{PATH}\\;QT_QUICK_BACKEND=software"
)
endif ()

if (WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(DEBUG_SUFFIX)
    if (MSVC AND CMAKE_BUILD_TYPE MATCHES "Debug")
        set(DEBUG_SUFFIX "d")
    endif ()
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_PATH "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    get_filename_component(QT_INSTALL_PATH "${QT_BIN_PATH}" DIRECTORY)
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
    if (MINGW)
        get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        foreach (MINGW_RUNTIME libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
            add_custom_command(TARGET HaikenAnime POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                    "${MINGW_BIN_DIR}/${MINGW_RUNTIME}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
        endforeach ()
    endif ()

    # DXC is not deployed by windeployqt, but Qt's Windows deployment can
    # require these DirectX Shader Compiler runtime libraries. Keep them next
    # to the executable so the packaged application can load them reliably.
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(DXC_ARCH x64)
    else ()
        set(DXC_ARCH x86)
    endif ()
    set(DXC_REDIST_DIR
            "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/${DXC_ARCH}")
    foreach (DXC_RUNTIME dxcompiler.dll dxil.dll)
        if (EXISTS "${DXC_REDIST_DIR}/${DXC_RUNTIME}")
            add_custom_command(TARGET HaikenAnime POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DXC_REDIST_DIR}/${DXC_RUNTIME}"
                    "$<TARGET_FILE_DIR:HaikenAnime>/${DXC_RUNTIME}"
                    COMMENT "Deploying ${DXC_RUNTIME}")
        else ()
            message(WARNING
                    "${DXC_RUNTIME} was not found in ${DXC_REDIST_DIR}")
        endif ()
    endforeach ()

    if (EXISTS "${QT_INSTALL_PATH}/bin/windeployqt.exe")
        add_custom_command(TARGET HaikenAnime POST_BUILD
                COMMAND "${QT_INSTALL_PATH}/bin/windeployqt.exe"
                --no-translations
                --compiler-runtime
                --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/resources/qml"
                "$<TARGET_FILE:HaikenAnime>"
                COMMENT "Deploying Qt and QML runtime dependencies")
    endif ()
endif ()
