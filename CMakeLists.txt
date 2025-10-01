cmake_minimum_required(VERSION 3.12)

project(pineapple-music LANGUAGES CXX VERSION 0.4.0)

include (GNUInstallDirs)
include (FeatureSummary)
include (FetchContent)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_QTEXTCODEC "Use QTextCodec instead of QStringConverter, in case Qt is not built with ICU" OFF)

find_package(Qt6 6.6 COMPONENTS Widgets Multimedia Network LinguistTools REQUIRED)
find_package(FFmpeg COMPONENTS avutil avformat)
find_package(TagLib 2.0.0)
find_package(KF6Codecs 6.1.0)

FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft.git
    GIT_TAG f5f2a3b2f2cd02bf80639adb12cbeed125bdf420
)
set(KISSFFT_PKGCONFIG OFF CACHE BOOL "dep(kissfft): pkgconfig support")
set(KISSFFT_STATIC ON CACHE BOOL "dep(kissfft): static linking")
set(KISSFFT_TEST OFF CACHE BOOL "dep(kissfft): enable testing")
set(KISSFFT_TOOLS OFF CACHE BOOL "dep(kissfft): build tools")
FetchContent_MakeAvailable(kissfft)

if (USE_QTEXTCODEC)
    find_package(Qt6 6.6 COMPONENTS Core5Compat REQUIRED)
endif()

set (PMUSIC_CPP_FILES
    main.cpp
    mainwindow.cpp
    seekableslider.cpp
    playlistmanager.cpp
    singleapplicationmanager.cpp
    lrcbar.cpp
    lyricsmanager.cpp
    fftspectrum.cpp
    playbackprogressindicator.cpp
)

set (PMUSIC_HEADER_FILES
    mainwindow.h
    seekableslider.h
    playlistmanager.h
    singleapplicationmanager.h
    lrcbar.h
    lyricsmanager.h
    fftspectrum.h
    playbackprogressindicator.h
    taskbarmanager.h
)

set (PMUSIC_UI_FILES
    mainwindow.ui
)

set (EXE_NAME pmusic)

# Translation
file (GLOB PMUSIC_TS_FILES languages/*.ts)
set (PMUSIC_CPP_FILES_FOR_I18N ${PMUSIC_CPP_FILES} ${PMUSIC_UI_FILES})

add_executable(${EXE_NAME}
    ${PMUSIC_HEADER_FILES}
    ${PMUSIC_CPP_FILES}
    ${PMUSIC_UI_FILES}
    resources.qrc
)

qt_add_translations(${EXE_NAME}
TS_FILES
    ${PMUSIC_TS_FILES}
)

if (WIN32)
    target_sources(${EXE_NAME} PRIVATE assets/pineapple-music.rc)
    target_sources(${EXE_NAME} PRIVATE taskbarmanager.cpp)
else ()
    target_sources(${EXE_NAME} PRIVATE taskbarmanager_dummy.cpp)
endif ()

if (NOT TagLib_FOUND)
    target_compile_definitions(${EXE_NAME} PRIVATE NO_TAGLIB=1)
else ()
    target_link_libraries(${EXE_NAME} PRIVATE TagLib::tag)
endif ()

if (TARGET KF6::Codecs)
    target_compile_definitions(${EXE_NAME} PRIVATE HAVE_KCODECS=1)
    target_link_libraries (${EXE_NAME} PRIVATE KF6::Codecs)
endif ()

if (FFmpeg_FOUND)
    target_compile_definitions(${EXE_NAME} PRIVATE HAVE_FFMPEG=1)
    target_link_libraries (${EXE_NAME} PRIVATE FFmpeg::avutil FFmpeg::avformat)
endif ()

target_link_libraries(${EXE_NAME} PRIVATE Qt::Widgets Qt::Multimedia Qt::Network kissfft::kissfft)

if (USE_QTEXTCODEC)
    target_compile_definitions(${EXE_NAME} PRIVATE USE_QTEXTCODEC=1)
    target_link_libraries(${EXE_NAME} PRIVATE Qt6::Core5Compat)
endif()

# Install settings
if (WIN32)
    set_target_properties(${EXE_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
elseif (APPLE)
    set_source_files_properties(assets/icons/app-icon.icns PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${EXE_NAME} PUBLIC assets/icons/app-icon.icns)
    # See https://cmake.org/cmake/help/v3.15/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
    set_target_properties(${EXE_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Pineapple Music"
        MACOSX_BUNDLE_GUI_IDENTIFIER net.blumia.pineapple-music
        MACOSX_BUNDLE_ICON_FILE app-icon.icns # contains the .icns file name, *without* the path.
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    )
elseif (UNIX)
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX /usr)
    endif ()

    # install icon
    install(
        FILES icons/app-icon.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps"
        RENAME net.blumia.pineapple-music.svg
    )

    # install shortcut
    install(
        FILES dist/net.blumia.pineapple-music.desktop
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )
endif()

set (INSTALL_TARGETS_DEFAULT_ARGS
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Devel
)

install (
    TARGETS ${EXE_NAME}
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
