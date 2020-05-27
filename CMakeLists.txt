cmake_minimum_required(VERSION 3.5)

project(pineapple-music LANGUAGES CXX)

include (GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets Multimedia Network REQUIRED)
find_package(PkgConfig)

if (PKG_CONFIG_FOUND)
pkg_check_modules(TagLib REQUIRED taglib)
endif ()

set (EXE_NAME pmusic)

add_executable(${EXE_NAME}
  main.cpp
  mainwindow.cpp
  mainwindow.h
  seekableslider.cpp
  seekableslider.h
  playlistmodel.h
  playlistmodel.cpp
  singleapplicationmanager.h
  singleapplicationmanager.cpp
  mainwindow.ui
  resources.qrc

  # 3rd party code
  FlacPic.h
  ID3v2Pic.h
)

if (NOT TagLib_FOUND)
    message (WARNING "TagLib not found!")
    target_compile_definitions(${EXE_NAME} PRIVATE
        NO_TAGLIB=1
    )
endif ()

target_include_directories(${EXE_NAME} PRIVATE ${TagLib_INCLUDE_DIRS})
target_link_libraries(${EXE_NAME} PRIVATE Qt5::Widgets Qt5::Multimedia Qt5::Network ${TagLib_LINK_LIBRARIES})

# Extra build settings
if (WIN32)
    set_property (
        TARGET ${EXE_NAME}
        PROPERTY WIN32_EXECUTABLE true
    )
endif ()

# Helper macros for install settings
macro (pmusic_convert_to_relative_path _var)
    # Make sure _var is a relative path
    if (IS_ABSOLUTE "${${_var}}")
        file (RELATIVE_PATH ${_var} "${CMAKE_INSTALL_PREFIX}" "${${_var}}")
    endif ()
endmacro ()

# Install settings
if (WIN32)
    # FIXME: try to avoid install to a "bin" subfolder under windows...
    #        when fixed, don't forget to update the CI config file...
    set (BIN_INSTALL_DIR "") # seems useless, don't know why...
elseif (UNIX)
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX /usr)
    endif ()

    set (BIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}") # relative, usually "bin"
    pmusic_convert_to_relative_path(BIN_INSTALL_DIR)
    set (LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}") # "lib" or "lib64"
    pmusic_convert_to_relative_path(LIB_INSTALL_DIR)

    # install icon
    install (
        FILES icons/app-icon.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps"
        RENAME pineapple-music.svg
    )

    # install shortcut
    install (
        FILES pineapple-music.desktop
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
    )
endif()

set (INSTALL_TARGETS_DEFAULT_ARGS
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR} COMPONENT Devel
)

install (
    TARGETS ${EXE_NAME}
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)
