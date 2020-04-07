cmake_minimum_required(VERSION 3.5)

project(pineapple-music LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets Multimedia REQUIRED)

add_executable(pineapple-music
  main.cpp
  mainwindow.cpp
  mainwindow.h
  seekableslider.cpp
  seekableslider.h
  playlistmodel.h
  playlistmodel.cpp
  mainwindow.ui
  resources.qrc
)

target_link_libraries(pineapple-music PRIVATE Qt5::Widgets Qt5::Multimedia)
