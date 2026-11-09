cmake_minimum_required(VERSION 3.21)

project(Quokka
    VERSION 0.9.0
    DESCRIPTION "A playful, read-only Lemmy (and Reddit) client for desktop and Android"
    HOMEPAGE_URL "https://github.com/sadikBr/Reddit-Client"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 Svg Multimedia Sql)
qt_standard_project_setup(REQUIRES 6.8)

option(QUOKKA_BUILD_TESTS "Build the unit tests" ON)
option(QUOKKA_DEPLOY_QT "Bundle Qt libraries on install (Windows/macOS packages)" OFF)

# ---- Core library: networking + parsing, no UI ------------------------------------------
add_subdirectory(src/core)

# ---- App ---------------------------------------------------------------------------------
qt_add_executable(quokka
    src/app/main.cpp
    src/app/services.h
    src/app/services.cpp
    src/app/networkfactory.h
    src/app/networkfactory.cpp
)

set(QUOKKA_QML_FILES
    qml/Main.qml
    qml/Nav.qml
    qml/theme/Theme.qml
    qml/pages/AboutPage.qml
    qml/pages/FeedPage.qml
    qml/pages/LocalPostsPage.qml
    qml/pages/PostPage.qml
    qml/pages/MediaViewer.qml
    qml/pages/MediaFeedPage.qml
    qml/pages/SearchPage.qml
    qml/pages/SettingsPage.qml
    qml/pages/UserPage.qml
    qml/components/Avatar.qml
    qml/components/Chip.qml
    qml/components/CommentRow.qml
    qml/components/DrawerContent.qml
    qml/components/EmptyState.qml
    qml/components/Icon.qml
    qml/components/ImportDialog.qml
    qml/components/JoinButton.qml
    qml/components/LoadingDots.qml
    qml/components/MediaStrip.qml
    qml/components/PlainField.qml
    qml/components/PostCard.qml
    qml/components/PostMedia.qml
    qml/components/RedditText.qml
    qml/components/RoundButton2.qml
    qml/components/RoundedImage.qml
    qml/components/SegmentedTabs.qml
    qml/components/ShortcutsHelp.qml
    qml/components/SettingsDrawer.qml
    qml/components/SignInDialog.qml
    qml/components/SkeletonCard.qml
    qml/components/SortBar.qml
    qml/components/SubredditPicker.qml
    qml/components/SubredditRow.qml
    qml/components/Toggle.qml
    qml/components/TopBar.qml
    qml/components/UserCommentCard.qml
    qml/components/VideoPlayer.qml
    qml/components/VideoThumb.qml
    qml/components/ThinScrollBar.qml
    qml/components/WheelScroller.qml
    qml/components/ZoomableImage.qml
)
# QML files live in sub-folders for tidiness, but are served flat at the module root so they can
# all see each other without imports.
foreach(qml_file IN LISTS QUOKKA_QML_FILES)
    get_filename_component(qml_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${qml_name})
endforeach()
set_source_files_properties(qml/theme/Theme.qml qml/Nav.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

file(GLOB QUOKKA_ICONS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} CONFIGURE_DEPENDS resources/icons/*.svg)

qt_add_qml_module(quokka
    URI Quokka
    VERSION 1.0
    QML_FILES ${QUOKKA_QML_FILES}
    SOURCES
        src/app/account.h src/app/account.cpp
        src/app/apiinfo.h src/app/apiinfo.cpp
        src/app/appsettings.h src/app/appsettings.cpp
        src/app/format.h src/app/format.cpp
        src/app/postlistmodel.h src/app/postlistmodel.cpp
        src/app/commenttreemodel.h src/app/commenttreemodel.cpp
        src/app/postmap.h src/app/postmap.cpp
        src/app/profiles.h src/app/profiles.cpp
        src/app/subredditlistmodel.h src/app/subredditlistmodel.cpp
        src/app/usercommentmodel.h src/app/usercommentmodel.cpp
        src/app/library.h src/app/library.cpp
        src/app/mediaresolver.h src/app/mediaresolver.cpp
        src/app/safearea.h src/app/safearea.cpp
    RESOURCES
        resources/logo.svg
        resources/fonts/Inter-Regular.ttf
        resources/fonts/Inter-Italic.ttf
        resources/fonts/Inter-Medium.ttf
        resources/fonts/Inter-SemiBold.ttf
        resources/fonts/Inter-Bold.ttf
        ${QUOKKA_ICONS}
)

target_include_directories(quokka PRIVATE src/app)
target_compile_definitions(quokka PRIVATE QUOKKA_VERSION="${PROJECT_VERSION}")
target_link_libraries(quokka PRIVATE
    quokka_core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Multimedia
    Qt6::Sql
)

set_target_properties(quokka PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.sadikbr.quokka
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

# ---- Android -----------------------------------------------------------------------------
if(ANDROID)
    set_target_properties(quokka PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
        QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE 1
        QT_ANDROID_MIN_SDK_VERSION 28
        QT_ANDROID_TARGET_SDK_VERSION 35
    )

    # Qt's Android build does not ship OpenSSL; without it every HTTPS request fails.
    include(FetchContent)
    FetchContent_Declare(android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP true
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
    add_android_openssl_libraries(quokka)
endif()

# ---- Install / deploy (desktop) ------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS quokka
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if(UNIX AND NOT APPLE AND NOT ANDROID)
    install(FILES resources/app/io.github.sadikbr.quokka.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES resources/app/quokka.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
endif()
if(NOT ANDROID AND QUOKKA_DEPLOY_QT)
    qt_generate_deploy_qml_app_script(
        TARGET quokka
        OUTPUT_SCRIPT quokka_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${quokka_deploy_script})
endif()

# ---- Tests -------------------------------------------------------------------------------
if(QUOKKA_BUILD_TESTS AND NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif()
