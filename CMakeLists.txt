cmake_minimum_required(VERSION 3.12)

project(pineapple-music LANGUAGES CXX)

include (GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5.1 COMPONENTS Widgets Multimedia Network LinguistTools REQUIRED)
find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
pkg_check_modules(TagLib taglib IMPORTED_TARGET)
endif ()

set (PMUSIC_CPP_FILES
    main.cpp
    mainwindow.cpp
    seekableslider.cpp
    playlistmanager.cpp
    singleapplicationmanager.cpp
)

set (PMUSIC_HEADER_FILES
    mainwindow.h
    seekableslider.h
    playlistmanager.h
    singleapplicationmanager.h
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
    message (WARNING "TagLib not found!")
    target_compile_definitions(${EXE_NAME} PRIVATE
        NO_TAGLIB=1
    )
else ()
    target_link_libraries(${EXE_NAME} PRIVATE PkgConfig::TagLib)
endif ()

target_link_libraries(${EXE_NAME} PRIVATE Qt::Widgets Qt::Multimedia Qt::Network)

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
