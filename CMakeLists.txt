cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/libqnapi/src/version.h" _qnapi_version_line
     REGEX "^#define QNAPI_VERSION ")
string(REGEX REPLACE "^#define QNAPI_VERSION \"([0-9.]+)\".*$" "\\1" QNAPI_VERSION "${_qnapi_version_line}")

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/libqnapi/src/version.h" _qnapi_displayable_line
     REGEX "^#define QNAPI_DISPLAYABLE_VERSION ")
string(REGEX REPLACE "^#define QNAPI_DISPLAYABLE_VERSION \"([^\"]+)\".*$" "\\1"
       _qnapi_displayable_default "${_qnapi_displayable_line}")

# Empty uses version.h. Release builds pass e.g. -DQNAPI_DISPLAYABLE_VERSION=0.3.0-rc1
# so About, --version and package names show the prerelease tag while PROJECT_VERSION
# stays the numeric QNAPI_VERSION (required by CMake).
set(QNAPI_DISPLAYABLE_VERSION "" CACHE STRING
    "Version shown in About, --version and packages; empty uses version.h")
if(QNAPI_DISPLAYABLE_VERSION STREQUAL "")
  set(QNAPI_DISPLAYABLE_VERSION "${_qnapi_displayable_default}")
endif()

project(QNapi VERSION ${QNAPI_VERSION} LANGUAGES CXX)

message(STATUS "QNapi ${QNAPI_DISPLAYABLE_VERSION} (QNAPI_VERSION ${PROJECT_VERSION})")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

option(QNAPI_BUILD_GUI "Build the qnapi graphical application" ON)
option(QNAPI_BUILD_CLI "Build the qnapic command-line application" ON)
set(QNAPI_QT_MAJOR_VERSION "" CACHE STRING
    "Qt major version to build with (5 or 6); empty prefers Qt 6")

include(CTest)
include(GNUInstallDirs)

if(NOT QNAPI_QT_MAJOR_VERSION)
  find_package(Qt6 QUIET COMPONENTS Core)
  if(Qt6_FOUND)
    set(QNAPI_QT_MAJOR_VERSION 6)
  else()
    set(QNAPI_QT_MAJOR_VERSION 5)
  endif()
endif()
set(QT_VERSION_MAJOR ${QNAPI_QT_MAJOR_VERSION})
set(QT Qt${QT_VERSION_MAJOR})

if(QT_VERSION_MAJOR EQUAL 5)
  set(QNAPI_QT_MIN_VERSION 5.15)
else()
  set(QNAPI_QT_MIN_VERSION 6.2)
endif()

set(_qt_components Core Network LinguistTools)
if(QT_VERSION_MAJOR EQUAL 6)
  list(APPEND _qt_components Core5Compat)
endif()
if(BUILD_TESTING)
  list(APPEND _qt_components Test)
endif()
if(QNAPI_BUILD_GUI)
  list(APPEND _qt_components Widgets)
  if(APPLE)
    list(APPEND _qt_components Xml)
  endif()
endif()
find_package(${QT} ${QNAPI_QT_MIN_VERSION} REQUIRED COMPONENTS ${_qt_components})
message(STATUS "Building QNapi ${PROJECT_VERSION} with Qt ${${QT}_VERSION}")

# not for the resource compiler, which builds the generated qnapi.rc
if(MSVC)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/W3;/utf-8>")
else()
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra>")
endif()

# Everything lands in one directory so the GUI finds qnapic next to itself
# when run from the build tree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(WIN32)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # win32/QNapi-setup.nsi packages win32/out
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/win32/out" CACHE PATH
        "Install path prefix" FORCE)
  endif()
  set(QNAPI_INSTALL_BINDIR .)
  set(QNAPI_INSTALL_DATADIR .)
else()
  set(QNAPI_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
  set(QNAPI_INSTALL_DATADIR ${CMAKE_INSTALL_DATADIR})
endif()

include(cmake/QNapiTranslations.cmake)
include(cmake/QNapiThirdParty.cmake)

string(REPLACE "." ";" _qnapi_ver_parts "${PROJECT_VERSION}")
list(LENGTH _qnapi_ver_parts _qnapi_ver_n)
list(GET _qnapi_ver_parts 0 QNAPI_VERSION_MAJOR)
list(GET _qnapi_ver_parts 1 QNAPI_VERSION_MINOR)
if(_qnapi_ver_n GREATER 2)
  list(GET _qnapi_ver_parts 2 QNAPI_VERSION_PATCH)
else()
  set(QNAPI_VERSION_PATCH 0)
endif()
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/qnapi.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/qnapi.rc"
    @ONLY)
set(QNAPI_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/qnapi.rc")

add_subdirectory(libqnapi)
if(QNAPI_BUILD_CLI)
  add_subdirectory(cli)
endif()
if(QNAPI_BUILD_GUI)
  add_subdirectory(gui)
endif()
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

include(cmake/QNapiInstall.cmake)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
  add_custom_target(doxygen
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen"
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating Doxygen documentation in doc/doxygen/index.html"
    VERBATIM)
endif()
