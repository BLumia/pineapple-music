cmake_minimum_required(VERSION 3.12)

project(pineapple-music LANGUAGES CXX)

include (GNUInstallDirs)
include (FeatureSummary)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_QTEXTCODEC "Use QTextCodec instead of QStringConverter, in case Qt is not built with ICU" OFF)

find_package(Qt6 6.6 COMPONENTS Widgets Multimedia Network LinguistTools REQUIRED)
find_package(KF6Codecs 6.1.0)
find_package(PkgConfig)

if (USE_QTEXTCODEC)
    find_package(Qt6 6.6 COMPONENTS Core5Compat REQUIRED)
endif()

if (PKG_CONFIG_FOUND)
pkg_check_modules(TagLib taglib IMPORTED_TARGET)
endif ()

set (PMUSIC_CPP_FILES
    main.cpp
    mainwindow.cpp
    seekableslider.cpp
    playlistmanager.cpp
    singleapplicationmanager.cpp
    lrcbar.cpp
    lyricsmanager.cpp
)

set (PMUSIC_HEADER_FILES
    mainwindow.h
    seekableslider.h
    playlistmanager.h
    singleapplicationmanager.h
    lrcbar.h
    lyricsmanager.h
)

set (PMUSIC_UI_FILES
    mainwindow.ui
)

set (EXE_NAME pmusic)

# Translation
file (GLOB PMUSIC_TS_FILES languages/*.ts)
set (PMUSIC_CPP_FILES_FOR_I18N ${PMUSIC_CPP_FILES} ${PMUSIC_UI_FILES})

add_executable(${EXE_NAME} WIN32
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
endif ()

if (NOT TagLib_FOUND)
    target_compile_definitions(${EXE_NAME} PRIVATE NO_TAGLIB=1)
else ()
    target_link_libraries(${EXE_NAME} PRIVATE PkgConfig::TagLib)
endif ()

if (TARGET KF6::Codecs)
    target_compile_definitions(${EXE_NAME} PRIVATE HAVE_KCODECS=1)
    target_link_libraries (${EXE_NAME} PRIVATE KF6::Codecs)
endif ()

target_link_libraries(${EXE_NAME} PRIVATE Qt::Widgets Qt::Multimedia Qt::Network)

if (USE_QTEXTCODEC)
    target_compile_definitions(${EXE_NAME} PRIVATE USE_QTEXTCODEC=1)
    target_link_libraries(${EXE_NAME} PRIVATE Qt6::Core5Compat)
endif()

# Install settings
if (WIN32)
    # FIXME: try to avoid install to a "bin" subfolder under windows...
    #        when fixed, don't forget to update the CI config file...
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
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Devel
)

install (
    TARGETS ${EXE_NAME}
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
