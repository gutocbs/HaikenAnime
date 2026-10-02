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

qt_add_executable(HaikenAnime main.cpp
        src/app/ApplicationComposition.h
        src/app/ApplicationComposition.cpp
        src/app/InitialSyncCoordinator.h
        src/app/InitialSyncCoordinator.cpp
        src/app/LocalLibraryScanCoordinator.h
        src/app/LocalLibraryScanCoordinator.cpp
        src/app/SeasonalCatalogCoordinator.h
        src/app/SeasonalCatalogCoordinator.cpp
        src/app/TranslationLoader.h
        src/app/TranslationLoader.cpp
        src/domain/media/MediaLink.h
        src/domain/media/Media.h
        src/application/media/MediaPage.h
        src/application/media/MediaSyncFilter.h
        src/application/media/MediaTitleResolver.h
        src/application/media/MediaTitleResolver.cpp
        src/application/media/MediaDetailsPresentation.h
        src/application/media/MediaDetailsPresentation.cpp
        src/application/catalog/SeasonalCatalogTypes.h
        src/application/catalog/ISeasonalCatalogDataSource.h
        src/domain/media/MediaType.h
        src/domain/media/MediaStatus.h
        src/domain/media/UserListStatus.h
        src/application/anilist/AniListCredentials.h
        src/infrastructure/anilist/AniListGraphQlError.h
        src/infrastructure/anilist/AniListGraphQlResponse.h
        src/infrastructure/anilist/AniListGraphQlResponseParser.h
        src/infrastructure/anilist/AniListGraphQlResponseParser.cpp
        src/infrastructure/anilist/AniListGraphQlPageParser.h
        src/infrastructure/anilist/AniListGraphQlPageParser.cpp
        src/infrastructure/anilist/AniListMediaDto.h
        src/domain/anilist/AniListField.h
        src/domain/anilist/AniListFieldValue.h
        src/domain/anilist/AniListMergePolicy.h
        src/domain/anilist/AniListMergeResult.h
        src/application/anilist/AniListPendingChangeStatus.h
        src/application/anilist/AniListPendingChange.h
        src/domain/anilist/AniListMergeDecision.h
        src/application/configuration/AniListSettings.h
        src/application/configuration/HttpSettings.h
        src/application/configuration/Settings.h
        src/application/configuration/UserPreferences.h
        src/application/configuration/CardStatusPresentation.h
        src/application/configuration/LanguagePreference.h
        src/application/configuration/UserPreferencesValidator.h
        src/application/configuration/UserPreferencesValidator.cpp
        src/application/covers/CoverQuality.h
        src/application/covers/CoverSettings.h
        src/application/covers/CoverSourceResolver.h
        src/application/covers/CoverSourceResolver.cpp
        src/application/covers/CoverCacheEntry.h
        src/application/covers/ICoverCacheRepository.h
        src/application/covers/ICoverFileStore.h
        src/application/covers/CoverDownloadTypes.h
        src/application/covers/ICoverDownloader.h
        src/application/covers/CoverDownloadCoordinator.h
        src/application/covers/CoverDownloadCoordinator.cpp
        src/application/configuration/ISettingsReader.h
        src/application/anilist/ISecretStore.h
        src/application/anilist/IAniListAuthProvider.h
        src/application/media/IMediaRepository.h
        src/application/media/IMediaReader.h
        src/application/media/IMediaWriter.h
        src/application/media/IMediaSnapshotReconciler.h
        src/application/anilist/AniListSyncService.h
        src/application/anilist/AniListSyncService.cpp
        src/application/anilist/AniListSyncErrorClassifier.h
        src/application/anilist/AniListSyncErrorClassifier.cpp
        src/application/anilist/AniListSyncErrorCategory.h
        src/application/anilist/AniListPendingChangeProcessor.h
        src/application/anilist/AniListPendingChangeProcessor.cpp
        src/application/anilist/AniListPendingChangeCompactor.h
        src/application/anilist/AniListPendingChangeCompactor.cpp
        src/application/anilist/AniListPendingChangeGrouper.h
        src/application/anilist/AniListPendingChangeGrouper.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
        src/application/anilist/AniListSyncWorker.h
        src/application/anilist/AniListSyncWorker.cpp
        src/application/anilist/AniListPendingChangeFactory.h
        src/application/anilist/AniListPendingChangeFactory.cpp
        src/application/anilist/AniListMergeService.h
        src/application/anilist/AniListMergeService.cpp
        src/application/anilist/IPendingChangeRepository.h
        src/application/anilist/IAniListUpdateClient.h
        src/infrastructure/anilist/AniListUpdateClient.h
        src/infrastructure/anilist/AniListUpdateClient.cpp
        src/application/anilist/AniListMediaPendingChanges.h
        src/application/anilist/AniListUpdateWorker.h
        src/application/anilist/AniListUpdateWorker.cpp
        src/application/anilist/AniListAuthManager.h
        src/application/anilist/AniListAuthManager.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteMediaRepository.h
        src/infrastructure/database/SqliteMediaRepository.cpp
        src/infrastructure/database/SqlitePendingChangeRepository.h
        src/infrastructure/database/SqlitePendingChangeRepository.cpp
        src/infrastructure/database/SqliteMediaMapper.h
        src/infrastructure/database/SqliteMediaMapper.cpp
        src/infrastructure/database/SqliteCoverCacheRepository.h
        src/infrastructure/database/SqliteCoverCacheRepository.cpp
        src/domain/library/LocalFileRecord.h
        src/application/library/ILocalFileRepository.h
        src/application/library/LocalLibraryScanTypes.h
        src/application/library/ILocalLibraryScanner.h
        src/infrastructure/library/IDirectoryEnumerator.h
        src/infrastructure/library/QtDirectoryEnumerator.h
        src/infrastructure/library/QtDirectoryEnumerator.cpp
        src/infrastructure/library/LocalLibraryScanner.h
        src/infrastructure/library/LocalLibraryScanner.cpp
        src/infrastructure/database/SqliteLocalFileRepository.h
        src/infrastructure/database/SqliteLocalFileRepository.cpp
        src/infrastructure/database/SqliteUserPreferencesRepository.h
        src/infrastructure/database/SqliteUserPreferencesRepository.cpp
        src/infrastructure/database/SqlQueryStore.h
        src/infrastructure/database/SqlQueryStore.cpp
        src/infrastructure/database/SqliteQueryConfiguration.h
        src/infrastructure/database/SqliteQueryConfiguration.cpp
        src/infrastructure/covers/CoverFileStore.h
        src/infrastructure/covers/CoverFileStore.cpp
        src/infrastructure/covers/CoverTemporaryStore.h
        src/infrastructure/covers/CoverTemporaryStore.cpp
        src/infrastructure/covers/QtCoverDownloader.h
        src/infrastructure/covers/QtCoverDownloader.cpp
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
        src/infrastructure/anilist/RecordedGraphQlAniListDataSource.h
        src/infrastructure/anilist/RecordedGraphQlAniListDataSource.cpp
        src/infrastructure/anilist/GraphQlAniListDataSource.h
        src/infrastructure/anilist/GraphQlAniListDataSource.cpp
        src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h
        src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
        src/infrastructure/configuration/JsonSettingsReader.h
        src/infrastructure/configuration/JsonSettingsReader.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
        src/presentation/home/HomeScreenController.h
        src/presentation/home/HomeScreenController.cpp
        src/presentation/seasonal/SeasonalCatalogController.h
        src/presentation/seasonal/SeasonalCatalogController.cpp
        src/presentation/settings/SettingsController.h
        src/presentation/settings/SettingsController.cpp
)

set_source_files_properties(
        resources/sqlite/sqlite-queries.json
        PROPERTIES QT_RESOURCE_ALIAS "sqlite-queries.json")
set_source_files_properties(
        resources/sqlite/queries/upsert-media.sql
        PROPERTIES QT_RESOURCE_ALIAS "upsert-media.sql")
set_source_files_properties(
        resources/sqlite/queries/read-media.sql
        PROPERTIES QT_RESOURCE_ALIAS "read-media.sql")
set_source_files_properties(
        resources/sqlite/queries/read-active-media-ids.sql
        PROPERTIES QT_RESOURCE_ALIAS "read-active-media-ids.sql")
set_source_files_properties(
        resources/sqlite/queries/mark-media-source-removed.sql
        PROPERTIES QT_RESOURCE_ALIAS "mark-media-source-removed.sql")
set_source_files_properties(
        resources/sqlite/queries/enqueue-pending-change.sql
        PROPERTIES QT_RESOURCE_ALIAS "enqueue-pending-change.sql")
set_source_files_properties(
        resources/sqlite/queries/read-pending-changes.sql
        PROPERTIES QT_RESOURCE_ALIAS "read-pending-changes.sql")
set_source_files_properties(
        resources/sqlite/queries/update-pending-change.sql
        PROPERTIES QT_RESOURCE_ALIAS "update-pending-change.sql")
set_source_files_properties(
        resources/sqlite/queries/read-cover-cache.sql
        PROPERTIES QT_RESOURCE_ALIAS "read-cover-cache.sql")
set_source_files_properties(
        resources/sqlite/queries/upsert-cover-cache.sql
        PROPERTIES QT_RESOURCE_ALIAS "upsert-cover-cache.sql")
set_source_files_properties(
        resources/sqlite/queries/delete-cover-cache.sql
        PROPERTIES QT_RESOURCE_ALIAS "delete-cover-cache.sql")
set_source_files_properties(
        resources/sqlite/queries/clear-cover-cache.sql
        PROPERTIES QT_RESOURCE_ALIAS "clear-cover-cache.sql")
set_source_files_properties(
        resources/sqlite/queries/read-user-preferences.sql
        PROPERTIES QT_RESOURCE_ALIAS "read-user-preferences.sql")
set_source_files_properties(
        resources/sqlite/queries/upsert-user-preferences.sql
        PROPERTIES QT_RESOURCE_ALIAS "upsert-user-preferences.sql")
set(LIBRARY_SCAN_SQL_RESOURCES
        resources/sqlite/queries/begin-library-scan.sql
        resources/sqlite/queries/upsert-local-file.sql
        resources/sqlite/queries/complete-library-scan.sql
        resources/sqlite/queries/fail-library-scan.sql
        resources/sqlite/queries/mark-local-files-unavailable.sql
)
foreach (QUERY_FILE IN LISTS LIBRARY_SCAN_SQL_RESOURCES)
    get_filename_component(QUERY_ALIAS "${QUERY_FILE}" NAME)
    set_source_files_properties("${QUERY_FILE}" PROPERTIES QT_RESOURCE_ALIAS "${QUERY_ALIAS}")
endforeach ()

set_source_files_properties(
        Settings.json
        PROPERTIES QT_RESOURCE_ALIAS "Settings.json")
set_source_files_properties(
        tests/fixtures/media-library.json
        PROPERTIES QT_RESOURCE_ALIAS "media-library.json")
set_source_files_properties(
        tests/fixtures/graphql/page-response.json
        PROPERTIES QT_RESOURCE_ALIAS "graphql/page-response.json")

qt_add_resources(HaikenAnime sqlite_resource_bundle
        PREFIX "/sqlite/queries"
        FILES
        ${LIBRARY_SCAN_SQL_RESOURCES}
        resources/sqlite/sqlite-queries.json
        resources/sqlite/queries/upsert-media.sql
        resources/sqlite/queries/read-media.sql
        resources/sqlite/queries/read-active-media-ids.sql
        resources/sqlite/queries/mark-media-source-removed.sql
        resources/sqlite/queries/enqueue-pending-change.sql
        resources/sqlite/queries/read-pending-changes.sql
        resources/sqlite/queries/update-pending-change.sql
        resources/sqlite/queries/read-cover-cache.sql
        resources/sqlite/queries/upsert-cover-cache.sql
        resources/sqlite/queries/delete-cover-cache.sql
        resources/sqlite/queries/clear-cover-cache.sql
        resources/sqlite/queries/read-user-preferences.sql
        resources/sqlite/queries/upsert-user-preferences.sql
)

qt_add_resources(HaikenAnime application_config_resource
        PREFIX "/config"
        FILES
        Settings.json
)

qt_add_resources(HaikenAnime fixture_resource_bundle
        PREFIX "/fixtures"
        FILES
        tests/fixtures/media-library.json
        tests/fixtures/graphql/page-response.json
)

set_source_files_properties(
        resources/anilist/queries/seasonal-catalog.graphql
        PROPERTIES QT_RESOURCE_ALIAS "seasonal-catalog.graphql")
qt_add_resources(HaikenAnime anilist_query_resource_bundle
        PREFIX "/anilist/queries"
        FILES resources/anilist/queries/seasonal-catalog.graphql)

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
        RESOURCES resources/images/cover-placeholder.svg
)

target_link_libraries(HaikenAnime
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Sql
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
add_executable(MediaDomainTests
        tests/unit/MediaDomainTests.cpp
        src/domain/media/MediaLink.h
        src/domain/media/Media.h
        src/domain/media/MediaStatus.h
        src/domain/media/UserListStatus.h
        src/domain/media/MediaType.h
)

target_link_libraries(MediaDomainTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME MediaDomainTests COMMAND MediaDomainTests)

add_executable(MediaTitleResolverTests
        tests/unit/MediaTitleResolverTests.cpp
        src/application/media/MediaTitleResolver.cpp
)
target_link_libraries(MediaTitleResolverTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME MediaTitleResolverTests COMMAND MediaTitleResolverTests)

add_executable(SqliteDatabaseTests
        tests/unit/SqliteDatabaseTests.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
)

target_link_libraries(SqliteDatabaseTests
        PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
)

add_test(NAME SqliteDatabaseTests COMMAND SqliteDatabaseTests)

add_executable(SqliteUserPreferencesRepositoryTests
        tests/unit/SqliteUserPreferencesRepositoryTests.cpp
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteUserPreferencesRepository.cpp
        src/application/configuration/UserPreferencesValidator.cpp
        src/application/media/MediaTitleResolver.cpp
        src/infrastructure/logging/AsyncLogger.cpp
)
target_link_libraries(SqliteUserPreferencesRepositoryTests PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
target_compile_definitions(SqliteUserPreferencesRepositoryTests PRIVATE HAIKENANIME_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME SqliteUserPreferencesRepositoryTests COMMAND SqliteUserPreferencesRepositoryTests)

add_executable(SqliteRepositoryTests
        tests/unit/SqliteRepositoryTests.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteMediaMapper.h
        src/infrastructure/database/SqliteMediaMapper.cpp
        src/infrastructure/database/SqliteMediaRepository.h
        src/infrastructure/database/SqliteMediaRepository.cpp
        src/infrastructure/database/SqlitePendingChangeRepository.h
        src/infrastructure/database/SqlitePendingChangeRepository.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
)

target_link_libraries(SqliteRepositoryTests
        PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
)

add_test(NAME SqliteRepositoryTests COMMAND SqliteRepositoryTests)

add_executable(SqliteCoverCacheRepositoryTests
        tests/unit/SqliteCoverCacheRepositoryTests.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteCoverCacheRepository.h
        src/infrastructure/database/SqliteCoverCacheRepository.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
)

target_link_libraries(SqliteCoverCacheRepositoryTests
        PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
)

add_test(NAME SqliteCoverCacheRepositoryTests COMMAND SqliteCoverCacheRepositoryTests)

add_executable(SqliteLocalFileRepositoryTests
        tests/unit/SqliteLocalFileRepositoryTests.cpp
        src/infrastructure/database/SqliteLocalFileRepository.cpp
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/logging/AsyncLogger.cpp
)
qt_add_resources(SqliteLocalFileRepositoryTests library_scan_test_resources
        PREFIX "/sqlite/queries"
        FILES ${LIBRARY_SCAN_SQL_RESOURCES}
)
target_link_libraries(SqliteLocalFileRepositoryTests PRIVATE Qt6::Core Qt6::Sql Qt6::Test)
add_test(NAME SqliteLocalFileRepositoryTests COMMAND SqliteLocalFileRepositoryTests)

add_executable(LocalLibraryScannerTests tests/unit/LocalLibraryScannerTests.cpp
        src/infrastructure/library/LocalLibraryScanner.cpp
        src/infrastructure/library/QtDirectoryEnumerator.cpp)
target_link_libraries(LocalLibraryScannerTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME LocalLibraryScannerTests COMMAND LocalLibraryScannerTests)

add_executable(SqlQueryStoreTests
        tests/unit/SqlQueryStoreTests.cpp
        src/infrastructure/database/SqlQueryStore.h
        src/infrastructure/database/SqlQueryStore.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
)

target_link_libraries(SqlQueryStoreTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME SqlQueryStoreTests COMMAND SqlQueryStoreTests)

add_executable(SqliteQueryConfigurationTests
        tests/unit/SqliteQueryConfigurationTests.cpp
        src/infrastructure/database/SqliteQueryConfiguration.h
        src/infrastructure/database/SqliteQueryConfiguration.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
)

qt_add_resources(SqliteQueryConfigurationTests sqlite_query_configuration_test_resources
        PREFIX "/sqlite/queries"
        FILES resources/sqlite/sqlite-queries.json ${LIBRARY_SCAN_SQL_RESOURCES}
)

target_link_libraries(SqliteQueryConfigurationTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME SqliteQueryConfigurationTests COMMAND SqliteQueryConfigurationTests)

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

add_executable(JsonSettingsReaderTests
        tests/unit/JsonSettingsReaderTests.cpp
        src/infrastructure/configuration/JsonSettingsReader.h
        src/infrastructure/configuration/JsonSettingsReader.cpp
        src/application/configuration/UserPreferencesValidator.cpp
        src/application/media/MediaTitleResolver.cpp
)

target_link_libraries(JsonSettingsReaderTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME JsonSettingsReaderTests COMMAND JsonSettingsReaderTests)
target_compile_definitions(JsonSettingsReaderTests PRIVATE
        HAIKENANIME_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(UserPreferencesValidatorTests
        tests/unit/UserPreferencesValidatorTests.cpp
        src/application/configuration/UserPreferencesValidator.cpp
        src/application/media/MediaTitleResolver.cpp
)
target_link_libraries(UserPreferencesValidatorTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME UserPreferencesValidatorTests COMMAND UserPreferencesValidatorTests)

add_executable(TranslationLoaderTests
        tests/unit/TranslationLoaderTests.cpp
        src/app/TranslationLoader.cpp
)
target_link_libraries(TranslationLoaderTests PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
qt_add_translations(TranslationLoaderTests
        TS_FILES ${HAIKENANIME_TS_FILES}
        RESOURCE_PREFIX "/i18n"
        LUPDATE_OPTIONS -no-obsolete
)
add_test(NAME TranslationLoaderTests COMMAND TranslationLoaderTests)

add_executable(CoverSettingsTests
        tests/unit/CoverSettingsTests.cpp
)

target_link_libraries(CoverSettingsTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME CoverSettingsTests COMMAND CoverSettingsTests)

add_executable(CoverSourceResolverTests
        tests/unit/CoverSourceResolverTests.cpp
        src/application/covers/CoverSourceResolver.cpp
)
target_link_libraries(CoverSourceResolverTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME CoverSourceResolverTests COMMAND CoverSourceResolverTests)

add_executable(CoverFileStoreTests
        tests/unit/CoverFileStoreTests.cpp
        src/infrastructure/covers/CoverFileStore.h
        src/infrastructure/covers/CoverFileStore.cpp
)

target_include_directories(CoverFileStoreTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(CoverFileStoreTests PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME CoverFileStoreTests COMMAND CoverFileStoreTests)

add_executable(CoverTemporaryStoreTests
        tests/unit/CoverTemporaryStoreTests.cpp
        src/infrastructure/covers/CoverTemporaryStore.h
        src/infrastructure/covers/CoverTemporaryStore.cpp
)
target_include_directories(CoverTemporaryStoreTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(CoverTemporaryStoreTests PRIVATE HAIKENANIME_COVER_TEMPORARY_STORE_TESTING)
target_link_libraries(CoverTemporaryStoreTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME CoverTemporaryStoreTests COMMAND CoverTemporaryStoreTests)

add_executable(CoverDownloadCoordinatorTests tests/unit/CoverDownloadCoordinatorTests.cpp
        src/application/covers/CoverDownloadCoordinator.cpp
        src/infrastructure/logging/AsyncLogger.cpp)
target_include_directories(CoverDownloadCoordinatorTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(CoverDownloadCoordinatorTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME CoverDownloadCoordinatorTests COMMAND CoverDownloadCoordinatorTests)

add_executable(QtCoverDownloaderTests tests/unit/QtCoverDownloaderTests.cpp
        src/infrastructure/covers/QtCoverDownloader.cpp)
target_include_directories(QtCoverDownloaderTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(QtCoverDownloaderTests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME QtCoverDownloaderTests COMMAND QtCoverDownloaderTests)

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
        src/application/anilist/AniListSyncErrorClassifier.h
        src/application/anilist/AniListSyncErrorClassifier.cpp
        src/application/anilist/AniListPendingChangeProcessor.h
        src/application/anilist/AniListPendingChangeProcessor.cpp
        src/application/anilist/AniListPendingChangeCompactor.h
        src/application/anilist/AniListPendingChangeCompactor.cpp
        src/application/anilist/AniListPendingChangeGrouper.h
        src/application/anilist/AniListPendingChangeGrouper.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
        src/application/media/IMediaRepository.h
        src/application/media/IMediaReader.h
        src/application/media/IMediaWriter.h
        src/application/media/IMediaDataSource.h
        src/infrastructure/anilist/FileAniListDataSource.h
        src/infrastructure/anilist/FileAniListDataSource.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
)

target_link_libraries(AniListFlowTests
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

target_compile_definitions(AniListFlowTests
        PRIVATE
        HAIKENANIME_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)

qt_add_resources(AniListFlowTests fixture_test_resource_bundle
        PREFIX "/fixtures"
        FILES
        tests/fixtures/media-library.json
)

add_test(NAME AniListFlowTests COMMAND AniListFlowTests)

add_executable(AniListGraphQlParsingTests
        tests/unit/AniListGraphQlParsingTests.cpp
        src/infrastructure/anilist/AniListGraphQlResponseParser.h
        src/infrastructure/anilist/AniListGraphQlResponseParser.cpp
        src/infrastructure/anilist/AniListGraphQlPageParser.h
        src/infrastructure/anilist/AniListGraphQlPageParser.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
)

target_link_libraries(AniListGraphQlParsingTests
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

target_compile_definitions(AniListGraphQlParsingTests
        PRIVATE
        HAIKENANIME_GRAPHQL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/graphql/page-response.json"
)

add_test(NAME AniListGraphQlParsingTests COMMAND AniListGraphQlParsingTests)

add_executable(GraphQlQueryStoreTests
        tests/unit/GraphQlQueryStoreTests.cpp
        src/infrastructure/anilist/GraphQlQueryStore.h
        src/infrastructure/anilist/GraphQlQueryStore.cpp
)

target_link_libraries(GraphQlQueryStoreTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME GraphQlQueryStoreTests COMMAND GraphQlQueryStoreTests)

add_executable(SeasonalCatalogTests
        tests/unit/SeasonalCatalogTests.cpp
        src/app/SeasonalCatalogCoordinator.h
        src/app/SeasonalCatalogCoordinator.cpp
        src/application/catalog/SeasonalCatalogTypes.h
        src/application/catalog/ISeasonalCatalogDataSource.h
        src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.h
        src/infrastructure/anilist/GraphQlSeasonalCatalogDataSource.cpp
        src/infrastructure/anilist/AniListGraphQlClient.h
        src/infrastructure/anilist/AniListGraphQlClient.cpp
        src/infrastructure/anilist/AniListGraphQlResponseParser.h
        src/infrastructure/anilist/AniListGraphQlResponseParser.cpp
        src/infrastructure/anilist/AniListGraphQlPageParser.h
        src/infrastructure/anilist/AniListGraphQlPageParser.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
        src/infrastructure/anilist/GraphQlQueryStore.h
        src/infrastructure/anilist/GraphQlQueryStore.cpp
)
target_include_directories(SeasonalCatalogTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(SeasonalCatalogTests PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Test)
target_compile_definitions(SeasonalCatalogTests PRIVATE
        HAIKENANIME_SEASONAL_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/graphql")
qt_add_resources(SeasonalCatalogTests seasonal_catalog_query_resource_bundle
        PREFIX "/anilist/queries"
        FILES resources/anilist/queries/seasonal-catalog.graphql)
add_test(NAME SeasonalCatalogTests COMMAND SeasonalCatalogTests)

add_executable(SeasonalCatalogControllerTests
        tests/unit/SeasonalCatalogControllerTests.cpp
        src/presentation/seasonal/SeasonalCatalogController.h
        src/presentation/seasonal/SeasonalCatalogController.cpp
        src/app/SeasonalCatalogCoordinator.h
        src/app/SeasonalCatalogCoordinator.cpp
        src/application/covers/CoverSourceResolver.cpp
        src/application/media/MediaTitleResolver.cpp
        src/application/media/MediaDetailsPresentation.cpp
)
target_include_directories(SeasonalCatalogControllerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(SeasonalCatalogControllerTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SeasonalCatalogControllerTests COMMAND SeasonalCatalogControllerTests)

add_executable(AniListSyncErrorClassifierTests
        tests/unit/AniListSyncErrorClassifierTests.cpp
        src/application/anilist/AniListSyncErrorClassifier.h
        src/application/anilist/AniListSyncErrorClassifier.cpp
)

target_link_libraries(AniListSyncErrorClassifierTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListSyncErrorClassifierTests COMMAND AniListSyncErrorClassifierTests)

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

add_executable(AniListMergeServiceTests
        tests/unit/AniListMergeServiceTests.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
        src/application/anilist/AniListMergeService.h
        src/application/anilist/AniListMergeService.cpp
)

target_link_libraries(AniListMergeServiceTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListMergeServiceTests COMMAND AniListMergeServiceTests)

add_executable(AniListPendingChangeProcessorTests
        tests/unit/AniListPendingChangeProcessorTests.cpp
        src/application/anilist/AniListPendingChangeProcessor.h
        src/application/anilist/AniListPendingChangeProcessor.cpp
        src/application/anilist/AniListPendingChangeCompactor.h
        src/application/anilist/AniListPendingChangeCompactor.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
        src/application/anilist/AniListMediaPendingChanges.h
        src/application/anilist/AniListPendingChangeGrouper.h
        src/application/anilist/AniListPendingChangeGrouper.cpp
)

target_link_libraries(AniListPendingChangeProcessorTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME AniListPendingChangeProcessorTests COMMAND AniListPendingChangeProcessorTests)

add_executable(HomeScreenControllerTests
        tests/unit/HomeScreenControllerTests.cpp
        src/presentation/home/HomeScreenController.h
        src/presentation/home/HomeScreenController.cpp
        src/application/covers/CoverDownloadCoordinator.h
        src/application/covers/CoverDownloadCoordinator.cpp
        src/application/covers/CoverSourceResolver.cpp
        src/application/media/MediaTitleResolver.cpp
        src/application/media/MediaDetailsPresentation.cpp
        src/infrastructure/logging/AsyncLogger.cpp
)

target_link_libraries(HomeScreenControllerTests
        PRIVATE
        Qt6::Core
        Qt6::Test
)
target_compile_definitions(HomeScreenControllerTests PRIVATE
        HAIKENANIME_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME HomeScreenControllerTests COMMAND HomeScreenControllerTests)

add_executable(SettingsControllerTests
        tests/unit/SettingsControllerTests.cpp
        src/presentation/settings/SettingsController.cpp
        src/application/configuration/UserPreferencesValidator.cpp
        src/application/media/MediaTitleResolver.cpp
        src/app/LocalLibraryScanCoordinator.h
        src/app/LocalLibraryScanCoordinator.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
)
target_link_libraries(SettingsControllerTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SettingsControllerTests COMMAND SettingsControllerTests)

add_executable(QmlStructureTests tests/unit/QmlStructureTests.cpp)
target_link_libraries(QmlStructureTests PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Test)
target_compile_definitions(QmlStructureTests PRIVATE
        HAIKENANIME_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
add_test(NAME QmlStructureTests COMMAND QmlStructureTests)

add_executable(InitialSyncCoordinatorTests
        tests/unit/InitialSyncCoordinatorTests.cpp
        src/app/InitialSyncCoordinator.h
        src/app/InitialSyncCoordinator.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
        src/infrastructure/database/SqliteDatabase.h
        src/infrastructure/database/SqliteDatabase.cpp
        src/infrastructure/database/SqliteMediaMapper.h
        src/infrastructure/database/SqliteMediaMapper.cpp
        src/infrastructure/database/SqliteMediaRepository.h
        src/infrastructure/database/SqliteMediaRepository.cpp
        src/infrastructure/database/SqlQueryStore.h
        src/infrastructure/database/SqlQueryStore.cpp
        src/infrastructure/anilist/FileAniListDataSource.h
        src/infrastructure/anilist/FileAniListDataSource.cpp
        src/infrastructure/anilist/RecordedGraphQlAniListDataSource.h
        src/infrastructure/anilist/RecordedGraphQlAniListDataSource.cpp
        src/infrastructure/anilist/AniListGraphQlResponseParser.h
        src/infrastructure/anilist/AniListGraphQlResponseParser.cpp
        src/infrastructure/anilist/AniListGraphQlPageParser.h
        src/infrastructure/anilist/AniListGraphQlPageParser.cpp
        src/infrastructure/anilist/AniListMediaMapper.h
        src/infrastructure/anilist/AniListMediaMapper.cpp
        src/application/anilist/AniListSyncService.h
        src/application/anilist/AniListSyncService.cpp
        src/application/anilist/AniListSyncErrorClassifier.h
        src/application/anilist/AniListSyncErrorClassifier.cpp
        src/application/anilist/AniListPendingChangeProcessor.h
        src/application/anilist/AniListPendingChangeProcessor.cpp
        src/application/anilist/AniListPendingChangeCompactor.h
        src/application/anilist/AniListPendingChangeCompactor.cpp
        src/application/anilist/AniListPendingChangeGrouper.h
        src/application/anilist/AniListPendingChangeGrouper.cpp
        src/application/anilist/AniListMergePolicyResolver.h
        src/application/anilist/AniListMergePolicyResolver.cpp
)

target_link_libraries(InitialSyncCoordinatorTests
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
        Qt6::Test
)

target_compile_definitions(InitialSyncCoordinatorTests PRIVATE
        HAIKENANIME_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_test(NAME InitialSyncCoordinatorTests COMMAND InitialSyncCoordinatorTests)

add_executable(LocalLibraryScanCoordinatorTests
        tests/unit/LocalLibraryScanCoordinatorTests.cpp
        src/app/LocalLibraryScanCoordinator.h
        src/app/LocalLibraryScanCoordinator.cpp
        src/infrastructure/logging/AsyncLogger.h
        src/infrastructure/logging/AsyncLogger.cpp
        src/infrastructure/library/LocalLibraryScanner.cpp
        src/infrastructure/library/QtDirectoryEnumerator.cpp)
target_link_libraries(LocalLibraryScanCoordinatorTests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME LocalLibraryScanCoordinatorTests COMMAND LocalLibraryScanCoordinatorTests)

get_target_property(COMPOSITION_SOURCES HaikenAnime SOURCES)
list(FILTER COMPOSITION_SOURCES INCLUDE REGEX "^src/")
add_executable(LocalLibraryScanCompositionTests tests/unit/LocalLibraryScanCompositionTests.cpp ${COMPOSITION_SOURCES})
file(GLOB COMPOSITION_SQL_RESOURCES CONFIGURE_DEPENDS resources/sqlite/queries/*.sql)
qt_add_resources(LocalLibraryScanCompositionTests composition_sql_resources PREFIX "/sqlite/queries"
        FILES resources/sqlite/sqlite-queries.json ${COMPOSITION_SQL_RESOURCES})
qt_add_resources(LocalLibraryScanCompositionTests composition_config_resources PREFIX "/config" FILES Settings.json)
target_link_libraries(LocalLibraryScanCompositionTests PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Test)
add_test(NAME LocalLibraryScanCompositionTests COMMAND LocalLibraryScanCompositionTests)

# Unit tests are separate executables and do not receive the deployment step
# used by the application target. Make Qt runtime DLLs available to CTest.
set(HAIKENANIME_TESTS
        MediaDomainTests
        MediaTitleResolverTests
        SqliteDatabaseTests
        SqliteUserPreferencesRepositoryTests
        SqliteRepositoryTests
        SqliteCoverCacheRepositoryTests
        SqliteLocalFileRepositoryTests
        LocalLibraryScannerTests
        SqlQueryStoreTests
        SqliteQueryConfigurationTests
        FileSecretStoreTests
        JsonSettingsReaderTests
        UserPreferencesValidatorTests
        TranslationLoaderTests
        CoverSettingsTests
        CoverSourceResolverTests
        CoverFileStoreTests
        CoverTemporaryStoreTests
        CoverDownloadCoordinatorTests
        QtCoverDownloaderTests
        AniListAuthManagerTests
        AniListFlowTests
        AniListGraphQlParsingTests
        GraphQlQueryStoreTests
        SeasonalCatalogTests
        SeasonalCatalogControllerTests
        AniListSyncErrorClassifierTests
        AniListMergePolicyResolverTests
        AniListPendingChangeFactoryTests
        AniListMergeServiceTests
        AniListPendingChangeProcessorTests
        HomeScreenControllerTests
        SettingsControllerTests
        QmlStructureTests
        InitialSyncCoordinatorTests
        LocalLibraryScanCoordinatorTests
        LocalLibraryScanCompositionTests
)
set_tests_properties(${HAIKENANIME_TESTS} PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        TIMEOUT 30
)
set_tests_properties(QmlStructureTests PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QUICK_BACKEND=software"
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
