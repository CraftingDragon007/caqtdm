cmake_minimum_required(VERSION 3.21)

project(caQtDM VERSION 4.6.0 LANGUAGES C CXX)

include(GNUInstallDirs)

set(CAQTDM_INSTALL_ROOT "." CACHE PATH "caQtDM flat install root")
set(CAQTDM_INSTALL_BINDIR "${CAQTDM_INSTALL_ROOT}" CACHE PATH "Install location for executables")
set(CAQTDM_INSTALL_LIBDIR "${CAQTDM_INSTALL_ROOT}" CACHE PATH "Install location for libraries")
set(CAQTDM_INSTALL_CONTROL_PLUGINDIR "${CAQTDM_INSTALL_ROOT}/controlsystems" CACHE PATH "Install location for control system plugins")
set(CAQTDM_INSTALL_DESIGNER_PLUGINDIR "${CAQTDM_INSTALL_ROOT}/designer" CACHE PATH "Install location for Qt Designer plugins")

string(REPLACE "." ";" _caqtdm_version_parts "${PROJECT_VERSION}")
list(LENGTH _caqtdm_version_parts _caqtdm_version_len)
if(_caqtdm_version_len LESS 3)
    list(APPEND _caqtdm_version_parts 0)
endif()
list(GET _caqtdm_version_parts 0 CAQTDM_VERSION_MAJOR)
list(GET _caqtdm_version_parts 1 CAQTDM_VERSION_MINOR)
list(GET _caqtdm_version_parts 2 CAQTDM_VERSION_PATCH)
set(CAQTDM_VERSION_STR "V${PROJECT_VERSION}")
set(CAQTDM_TARGET_COMPANY "Paul Scherrer Institut")
set(CAQTDM_TARGET_DESCRIPTION "Channel Access Qt Display Manager")
set(CAQTDM_TARGET_COPYRIGHT "Copyright (C) 2012-2025 Paul Scherrer Institut")
set(CAQTDM_TARGET_INTERNAL_NAME "caqtdm")

set(_caqtdm_build_processor "${CMAKE_SYSTEM_PROCESSOR}")
if(NOT _caqtdm_build_processor)
    set(_caqtdm_build_processor "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()
set(CAQTDM_BUILD_ARCH "${CMAKE_SYSTEM_NAME}-${_caqtdm_build_processor}")
if(WIN32)
    set(CAQTDM_BUILD_TIME "")
    set(CAQTDM_BUILD_DATE "")
else()
    string(TIMESTAMP CAQTDM_BUILD_TIME "%H:%M")
    string(TIMESTAMP CAQTDM_BUILD_DATE "%d-%m-%Y")
endif()
set(CAQTDM_SUPPORT_STRING "EPICS")

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(IOS TRUE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CaQtDMHelpers)
include(CaQtDMSubprojects)

if(NOT DEFINED CAQTDM_QT_PREFIX AND DEFINED ENV{QTHOME})
    set(CAQTDM_QT_PREFIX "$ENV{QTHOME}" CACHE PATH "Qt installation directory (QTHOME)")
endif()
if(CAQTDM_QT_PREFIX AND NOT "${CAQTDM_QT_PREFIX}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "${CAQTDM_QT_PREFIX}")
    message(STATUS "Using Qt prefix: ${CAQTDM_QT_PREFIX}")
endif()

if(NOT DEFINED CAQTDM_QWT_PREFIX AND DEFINED ENV{QWTHOME})
    set(CAQTDM_QWT_PREFIX "$ENV{QWTHOME}" CACHE PATH "Qwt installation directory (QWTHOME)")
endif()
if(CAQTDM_QWT_PREFIX AND NOT "${CAQTDM_QWT_PREFIX}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH "${CAQTDM_QWT_PREFIX}")
    message(STATUS "Using Qwt prefix: ${CAQTDM_QWT_PREFIX}")
endif()

# Provide a sensible default for EPICS base on typical Linux installs.
if(NOT DEFINED CAQTDM_EPICS_BASE OR "${CAQTDM_EPICS_BASE}" STREQUAL "")
    if(EXISTS "/usr/lib/epics")
        set(CAQTDM_EPICS_BASE "/usr/lib/epics")
    elseif(EXISTS "/usr/local/epics")
        set(CAQTDM_EPICS_BASE "/usr/local/epics")
    endif()
endif()

caqtdm_require_path_from_env(CAQTDM_EPICS_BASE EPICS_BASE "EPICS base installation (EPICS_BASE)")

# Derive the EPICS host architecture automatically when not provided.
if(NOT DEFINED CAQTDM_EPICS_HOST_ARCH OR "${CAQTDM_EPICS_HOST_ARCH}" STREQUAL "")
    set(_caqtdm_epics_lib_root "${CAQTDM_EPICS_BASE}/lib")
    if(IS_DIRECTORY "${_caqtdm_epics_lib_root}")
        file(GLOB _caqtdm_epics_arch_dirs RELATIVE "${_caqtdm_epics_lib_root}" "${_caqtdm_epics_lib_root}/*")
        list(FILTER _caqtdm_epics_arch_dirs INCLUDE REGEX "^[^\.].*")
        # Prefer platform-specific host directories based on current system
        set(_caqtdm_epics_arch_guess "")
        foreach(_arch IN LISTS _caqtdm_epics_arch_dirs)
            if(IS_DIRECTORY "${_caqtdm_epics_lib_root}/${_arch}")
                if(_caqtdm_epics_arch_guess STREQUAL "")
                    set(_caqtdm_epics_arch_guess "${_arch}")
                endif()
                # Prefer matching current platform
                if(WIN32 AND _arch MATCHES "windows|win32")
                    set(_caqtdm_epics_arch_guess "${_arch}")
                elseif(UNIX AND NOT APPLE AND _arch MATCHES "linux")
                    set(_caqtdm_epics_arch_guess "${_arch}")
                elseif(APPLE AND _arch MATCHES "darwin")
                    set(_caqtdm_epics_arch_guess "${_arch}")
                endif()
            endif()
        endforeach()
        if(NOT _caqtdm_epics_arch_guess STREQUAL "")
            set(CAQTDM_EPICS_HOST_ARCH "${_caqtdm_epics_arch_guess}")
        endif()
    endif()
endif()

caqtdm_require_string_from_env(CAQTDM_EPICS_HOST_ARCH EPICS_HOST_ARCH "EPICS host architecture (EPICS_HOST_ARCH)")
caqtdm_cache_path_from_env(CAQTDM_QWT_INCLUDE_DIR QWTINCLUDE "Qwt include directory" "")
caqtdm_cache_path_from_env(CAQTDM_QWT_LIBRARY_DIR QWTLIB "Qwt library directory" "")
caqtdm_cache_string_from_env(CAQTDM_QWT_LIBRARY_NAME QWTLIBNAME "Qwt library name" "")
caqtdm_cache_string_from_env(CAQTDM_QWT_VERSION_STRING QWTVERSION "Qwt version string" "")

find_package(Qwt QUIET)
if(Qwt_FOUND)
    if(NOT CAQTDM_QWT_INCLUDE_DIR AND Qwt_INCLUDE_DIRS)
        list(GET Qwt_INCLUDE_DIRS 0 _caqtdm_qwt_pkg_include)
        if(_caqtdm_qwt_pkg_include)
            set(CAQTDM_QWT_INCLUDE_DIR "${_caqtdm_qwt_pkg_include}" CACHE PATH "Qwt include directory" FORCE)
        endif()
    endif()
    if(NOT CAQTDM_QWT_LIBRARY AND Qwt_LIBRARIES)
        foreach(_caqtdm_qwt_pkg_lib IN LISTS Qwt_LIBRARIES)
            if(TARGET ${_caqtdm_qwt_pkg_lib})
                get_target_property(_caqtdm_qwt_pkg_lib_location ${_caqtdm_qwt_pkg_lib} IMPORTED_LOCATION)
                if(NOT _caqtdm_qwt_pkg_lib_location)
                    get_target_property(_caqtdm_qwt_pkg_lib_location ${_caqtdm_qwt_pkg_lib} IMPORTED_LOCATION_RELEASE)
                endif()
                if(_caqtdm_qwt_pkg_lib_location)
                    set(CAQTDM_QWT_LIBRARY "${_caqtdm_qwt_pkg_lib_location}" CACHE FILEPATH "Path to the Qwt library to link against" FORCE)
                    break()
                endif()
            elseif(EXISTS "${_caqtdm_qwt_pkg_lib}")
                set(CAQTDM_QWT_LIBRARY "${_caqtdm_qwt_pkg_lib}" CACHE FILEPATH "Path to the Qwt library to link against" FORCE)
                break()
            endif()
        endforeach()
    endif()
    if(NOT CAQTDM_QWT_VERSION_STRING AND DEFINED Qwt_VERSION_STRING AND NOT "${Qwt_VERSION_STRING}" STREQUAL "")
        set(CAQTDM_QWT_VERSION_STRING "${Qwt_VERSION_STRING}" CACHE STRING "Qwt version string" FORCE)
    endif()
endif()

set(_caqtdm_qt_core_components Core)
find_package(Qt6 COMPONENTS ${_caqtdm_qt_core_components} QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    set(_caqtdm_qt_package Qt6)
else()
    find_package(Qt5 COMPONENTS ${_caqtdm_qt_core_components} REQUIRED)
    set(QT_VERSION_MAJOR 5)
    set(_caqtdm_qt_package Qt5)
endif()
set(CAQTDM_QT_PACKAGE "${_caqtdm_qt_package}")

set(_qt_version_var "${_caqtdm_qt_package}Core_VERSION")
if(${_qt_version_var} VERSION_LESS "4.6.0")
    message(FATAL_ERROR "Cannot build caQtDM with Qt version ${${_qt_version_var}}. Use at least 4.6.")
endif()
set(CAQTDM_QT_VERSION_STRING "${${_qt_version_var}}")
string(REPLACE "." ";" _caqtdm_qt_version_parts "${CAQTDM_QT_VERSION_STRING}")
list(GET _caqtdm_qt_version_parts 0 CAQTDM_QT_VERSION_MAJOR)
list(LENGTH _caqtdm_qt_version_parts _caqtdm_qt_version_length)
if(_caqtdm_qt_version_length GREATER 1)
    list(GET _caqtdm_qt_version_parts 1 CAQTDM_QT_VERSION_MINOR)
else()
    set(CAQTDM_QT_VERSION_MINOR 0)
endif()
string(REGEX MATCH "^[0-9]+" CAQTDM_QT_VERSION_MINOR_NUM "${CAQTDM_QT_VERSION_MINOR}")
if(NOT CAQTDM_QT_VERSION_MINOR_NUM)
    set(CAQTDM_QT_VERSION_MINOR_NUM 0)
endif()

set(_default_mobile OFF)
if(ANDROID OR IOS)
    set(_default_mobile ON)
endif()
option(CAQTDM_ENABLE_MOBILE "Enable mobile-specific caQtDM features" ${_default_mobile})
if(ANDROID OR IOS)
    set(CAQTDM_ENABLE_MOBILE ON CACHE BOOL "Enable mobile-specific caQtDM features" FORCE)
endif()

if(CAQTDM_ENABLE_MOBILE)
    set(_caqtdm_adl_default OFF)
else()
    set(_caqtdm_adl_default ON)
endif()
option(CAQTDM_ENABLE_ADL_EDL "Enable ADL/EDL on-the-fly conversion" ${_caqtdm_adl_default})

if(CAQTDM_ENABLE_MOBILE)
    add_compile_definitions(MOBILE)
endif()
if(IOS)
    add_compile_definitions(MOBILE_IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "iOS architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "17.0" CACHE STRING "iOS deployment target" FORCE)
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "2")
elseif(ANDROID)
    add_compile_definitions(MOBILE_ANDROID)
endif()
if(CAQTDM_ENABLE_ADL_EDL)
    add_compile_definitions(ADL_EDL_FILES)
endif()
add_compile_definitions(IO_OPTIMIZED_FOR_TABWIDGETS)

if(NOT DEFINED CAQTDM_ANDROID_FUNCTIONS_INCLUDE AND DEFINED ENV{ANDROIDFUNCTIONSINCLUDE})
    set(CAQTDM_ANDROID_FUNCTIONS_INCLUDE "$ENV{ANDROIDFUNCTIONSINCLUDE}" CACHE PATH "Android functions include directory")
endif()

caqtdm_cache_path_from_env(CAQTDM_EPICS_INCLUDE_DIR EPICSINCLUDE "EPICS include directory" "${CAQTDM_EPICS_BASE}/include")
caqtdm_cache_path_from_env(CAQTDM_EPICS_LIBRARY_DIR EPICSLIB "EPICS library directory" "${CAQTDM_EPICS_BASE}/lib/${CAQTDM_EPICS_HOST_ARCH}")
caqtdm_cache_path_from_env(CAQTDM_EPICS_EXTENSIONS_DIR EPICSEXTENSIONS "EPICS extensions directory" "")
caqtdm_cache_path_from_env(CAQTDM_ZMQ_INCLUDE_DIR ZMQINC "ZeroMQ include directory" "/usr/include")
caqtdm_cache_path_from_env(CAQTDM_ZMQ_LIBRARY_DIR ZMQLIB "ZeroMQ library directory" "/usr/lib")
caqtdm_cache_string_from_env(CAQTDM_ZMQ_LIBRARY_NAME ZMQ_LIB_NAME "ZeroMQ library override" "")
caqtdm_cache_string_from_env(CAQTDM_PYTHON_VERSION PYTHONVERSION "Python version used for optional calculations" "3.11")
set(_caqtdm_default_python_include "/usr/include/python${CAQTDM_PYTHON_VERSION}")
caqtdm_cache_path_from_env(CAQTDM_PYTHON_INCLUDE_DIR PYTHONINCLUDE "Python include directory" "${_caqtdm_default_python_include}")
caqtdm_cache_path_from_env(CAQTDM_PYTHON_LIBRARY_DIR PYTHONLIB "Python library directory" "/usr/lib")
caqtdm_cache_path_from_env(CAQTDM_CA_ARCHIVE_LIB_DIR CAQTDM_CA_ARCHIVELIBS "Directory holding CA archive static libraries" "")
caqtdm_cache_path_from_env(CAQTDM_LOGGING_ARCHIVE_LIB_DIR CAQTDM_LOGGING_ARCHIVELIBS "Directory holding logging archive static libraries" "")

set(_caqtdm_qwt_include_hints)
if(CAQTDM_QWT_INCLUDE_DIR AND NOT "${CAQTDM_QWT_INCLUDE_DIR}" STREQUAL "")
    list(APPEND _caqtdm_qwt_include_hints "${CAQTDM_QWT_INCLUDE_DIR}")
endif()
if(CAQTDM_QWT_PREFIX)
    list(APPEND _caqtdm_qwt_include_hints
        "${CAQTDM_QWT_PREFIX}/include"
        "${CAQTDM_QWT_PREFIX}/include/qwt")
endif()
list(APPEND _caqtdm_qwt_include_hints /usr/include /usr/local/include)

if(NOT CAQTDM_QWT_INCLUDE_DIR OR "${CAQTDM_QWT_INCLUDE_DIR}" STREQUAL "")
    find_path(_caqtdm_qwt_include_dir
        NAMES qwt_plot.h
        HINTS ${_caqtdm_qwt_include_hints}
        PATH_SUFFIXES qwt qwt6 qwt-qt5 qwt-qt6)
    if(_caqtdm_qwt_include_dir)
        set(CAQTDM_QWT_INCLUDE_DIR "${_caqtdm_qwt_include_dir}" CACHE PATH "Qwt include directory" FORCE)
        set(CAQTDM_QWT_INCLUDE_DIR "${_caqtdm_qwt_include_dir}")
    endif()
endif()

if(NOT CAQTDM_QWT_INCLUDE_DIR)
    set(_caqtdm_qwt_known_include_dirs
        "/usr/include/qwt-qt6"
        "/usr/include/qwt-qt5"
        "/usr/include/qwt6"
        "/usr/include/qwt"
        "/usr/local/include/qwt-qt6"
        "/usr/local/include/qwt6"
        "/usr/local/include/qwt")
    foreach(_caqtdm_qwt_known_include_dir IN LISTS _caqtdm_qwt_known_include_dirs)
        if(EXISTS "${_caqtdm_qwt_known_include_dir}/qwt_plot.h")
            set(CAQTDM_QWT_INCLUDE_DIR "${_caqtdm_qwt_known_include_dir}" CACHE PATH "Qwt include directory" FORCE)
            set(CAQTDM_QWT_INCLUDE_DIR "${_caqtdm_qwt_known_include_dir}")
            break()
        endif()
    endforeach()
endif()

if(NOT CAQTDM_QWT_INCLUDE_DIR)
    message(FATAL_ERROR "Could not locate Qwt headers. Set CAQTDM_QWT_INCLUDE_DIR, QWTINCLUDE or QWTHOME.")
endif()

set(_caqtdm_qwt_library_hints)
if(CAQTDM_QWT_LIBRARY_DIR AND NOT "${CAQTDM_QWT_LIBRARY_DIR}" STREQUAL "")
    list(APPEND _caqtdm_qwt_library_hints "${CAQTDM_QWT_LIBRARY_DIR}")
endif()
if(CAQTDM_QWT_PREFIX)
    list(APPEND _caqtdm_qwt_library_hints
        "${CAQTDM_QWT_PREFIX}/lib"
        "${CAQTDM_QWT_PREFIX}/lib64")
endif()
list(APPEND _caqtdm_qwt_library_hints /usr/lib /usr/local/lib)

set(_caqtdm_qwt_library_names)
if(CAQTDM_QWT_LIBRARY_NAME AND NOT "${CAQTDM_QWT_LIBRARY_NAME}" STREQUAL "")
    list(APPEND _caqtdm_qwt_library_names "${CAQTDM_QWT_LIBRARY_NAME}")
endif()
list(APPEND _caqtdm_qwt_library_names qwt-qt6 qwt-qt5 qwt6 qwt)
list(REMOVE_DUPLICATES _caqtdm_qwt_library_names)

if(NOT CAQTDM_QWT_LIBRARY OR "${CAQTDM_QWT_LIBRARY}" STREQUAL "")
    find_library(_caqtdm_qwt_lib
        NAMES ${_caqtdm_qwt_library_names}
        HINTS ${_caqtdm_qwt_library_hints}
        PATH_SUFFIXES qwt6 qwt-qt5 qwt-qt6)
    if(_caqtdm_qwt_lib)
        set(CAQTDM_QWT_LIBRARY "${_caqtdm_qwt_lib}" CACHE FILEPATH "Path to the Qwt library to link against" FORCE)
        set(CAQTDM_QWT_LIBRARY "${_caqtdm_qwt_lib}")
    endif()
endif()

if(NOT CAQTDM_QWT_LIBRARY)
    set(_caqtdm_qwt_known_libraries
        "/usr/lib/libqwt-qt6.so"
        "/usr/lib64/libqwt-qt6.so"
        "/usr/lib/libqwt-qt5.so"
        "/usr/lib64/libqwt-qt5.so"
        "/usr/lib/libqwt6.so"
        "/usr/lib/libqwt.so"
        "/usr/local/lib/libqwt-qt6.so"
        "/usr/local/lib/libqwt.so")
    foreach(_caqtdm_qwt_known_lib IN LISTS _caqtdm_qwt_known_libraries)
        if(EXISTS "${_caqtdm_qwt_known_lib}")
            set(CAQTDM_QWT_LIBRARY "${_caqtdm_qwt_known_lib}" CACHE FILEPATH "Path to the Qwt library to link against" FORCE)
            set(CAQTDM_QWT_LIBRARY "${_caqtdm_qwt_known_lib}")
            break()
        endif()
    endforeach()
endif()

if(NOT CAQTDM_QWT_LIBRARY)
    message(FATAL_ERROR "Could not locate a Qwt library. Set CAQTDM_QWT_LIBRARY, QWTLIB, or QWTHOME.")
endif()

get_filename_component(_caqtdm_qwt_lib_dir "${CAQTDM_QWT_LIBRARY}" DIRECTORY)
set(CAQTDM_QWT_LIBRARY_DIR "${_caqtdm_qwt_lib_dir}" CACHE PATH "Qwt library directory" FORCE)

if(NOT CAQTDM_QWT_LIBRARY_NAME OR "${CAQTDM_QWT_LIBRARY_NAME}" STREQUAL "")
    get_filename_component(_caqtdm_qwt_lib_name "${CAQTDM_QWT_LIBRARY}" NAME_WE)
    set(CAQTDM_QWT_LIBRARY_NAME "${_caqtdm_qwt_lib_name}" CACHE STRING "Qwt library name" FORCE)
endif()

if(NOT CAQTDM_QWT_VERSION_STRING OR "${CAQTDM_QWT_VERSION_STRING}" STREQUAL "")
    set(_caqtdm_qwt_global_header "${CAQTDM_QWT_INCLUDE_DIR}/qwt_global.h")
    if(EXISTS "${_caqtdm_qwt_global_header}")
        file(STRINGS "${_caqtdm_qwt_global_header}" _caqtdm_qwt_version_line REGEX "^#define QWT_VERSION_STR")
        string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" _caqtdm_qwt_version "${_caqtdm_qwt_version_line}")
        if(_caqtdm_qwt_version AND NOT "${_caqtdm_qwt_version}" STREQUAL "")
            set(CAQTDM_QWT_VERSION_STRING "${_caqtdm_qwt_version}" CACHE STRING "Qwt version string" FORCE)
            set(CAQTDM_QWT_VERSION_STRING "${_caqtdm_qwt_version}")
        endif()
    endif()
endif()

if(NOT CAQTDM_QWT_VERSION_STRING)
    message(FATAL_ERROR "Could not determine Qwt version. Set CAQTDM_QWT_VERSION_STRING or QWTVERSION.")
endif()

if(NOT TARGET caqtdm::qwt)
    add_library(caqtdm::qwt UNKNOWN IMPORTED)
    set_target_properties(caqtdm::qwt PROPERTIES
        IMPORTED_LOCATION "${CAQTDM_QWT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CAQTDM_QWT_INCLUDE_DIR}")
endif()

string(REPLACE "." ";" _caqtdm_qwt_version_parts "${CAQTDM_QWT_VERSION_STRING}")
list(GET _caqtdm_qwt_version_parts 0 CAQTDM_QWT_VERSION_MAJOR)
list(LENGTH _caqtdm_qwt_version_parts _caqtdm_qwt_version_length)
if(_caqtdm_qwt_version_length GREATER 1)
    list(GET _caqtdm_qwt_version_parts 1 CAQTDM_QWT_VERSION_MINOR)
else()
    set(CAQTDM_QWT_VERSION_MINOR 0)
endif()
string(REGEX MATCH "^[0-9]+" CAQTDM_QWT_VERSION_MINOR_NUM "${CAQTDM_QWT_VERSION_MINOR}")
if(NOT CAQTDM_QWT_VERSION_MINOR_NUM)
    set(CAQTDM_QWT_VERSION_MINOR_NUM 0)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
    set(CAQTDM_QT_CORE5COMPAT_TARGET Qt6::Core5Compat)
else()
    set(CAQTDM_QT_CORE5COMPAT_TARGET "")
endif()

set(_caqtdm_enable_xdr_hack FALSE)
if(APPLE OR WIN32 OR IOS OR ANDROID)
    set(_caqtdm_enable_xdr_hack TRUE)
elseif(UNIX AND NOT APPLE)
    if((QT_VERSION_MAJOR EQUAL 5 AND CAQTDM_QT_VERSION_MINOR_NUM GREATER 9) OR QT_VERSION_MAJOR GREATER_EQUAL 6)
        set(_caqtdm_enable_xdr_hack TRUE)
    endif()
endif()

if(_caqtdm_enable_xdr_hack)
    add_compile_definitions(XDR_HACK XDR_LE)
endif()
set(CAQTDM_ENABLE_XDR_HACK ${_caqtdm_enable_xdr_hack} CACHE BOOL "Enable XDR support helpers" FORCE)

set(_caqtdm_enable_designer_tooltips FALSE)
if(QT_VERSION_MAJOR GREATER 5)
    set(_caqtdm_enable_designer_tooltips TRUE)
elseif(QT_VERSION_MAJOR EQUAL 5 AND CAQTDM_QT_VERSION_MINOR_NUM GREATER 5)
    set(_caqtdm_enable_designer_tooltips TRUE)
endif()
if(_caqtdm_enable_designer_tooltips)
    add_compile_definitions(DESIGNER_TOOLTIP_DESCRIPTIONS)
endif()

if(UNIX AND NOT APPLE)
    add_compile_definitions(linux)
endif()

caqtdm_bool_from_env(_caqtdm_env_modbus CAQTDM_MODBUS OFF)
caqtdm_bool_from_env(_caqtdm_env_gps CAQTDM_GPS OFF)
caqtdm_bool_from_env(_caqtdm_env_norpath CAQTDM_NORPATH OFF)
caqtdm_bool_from_env(_caqtdm_env_bsread CAQTDM_BSREAD ON)
set(_default_rpath ON)
if(_caqtdm_env_norpath)
    set(_default_rpath OFF)
endif()
option(CAQTDM_ENABLE_RPATH "Embed runtime search paths for third-party libraries" ${_default_rpath})

set(_default_pythoncalc ON)
if(WIN32)
    # Python is not typically available by default on Windows
    set(_default_pythoncalc OFF)
endif()
option(CAQTDM_ENABLE_PYTHONCALC "Enable optional Python-assisted calculations" ${_default_pythoncalc})
option(CAQTDM_ENABLE_ARCHIVE_PLUGINS "Build archive plugins" ON)

set(_default_bsread ${_caqtdm_env_bsread})
if(NOT CAQTDM_ENABLE_MOBILE)
    if(EXISTS "${CAQTDM_ZMQ_INCLUDE_DIR}/zmq.h")
        set(_default_bsread ON)
    endif()
else()
    set(_default_bsread OFF)
endif()
option(CAQTDM_ENABLE_BSREAD_PLUGIN "Build the bsread control system plugin" ${_default_bsread})

option(CAQTDM_ENABLE_MODBUS_PLUGIN "Build the Modbus control system plugin" ${_caqtdm_env_modbus})
option(CAQTDM_ENABLE_GPS_PLUGIN "Build the GPS control system plugin" ${_caqtdm_env_gps})

set(_default_archive_sf ON)
set(_default_archive_http OFF)
if(QT_VERSION_MAJOR GREATER 5)
    set(_default_archive_http ON)
elseif(QT_VERSION_MAJOR EQUAL 5 AND CAQTDM_QT_VERSION_MINOR_NUM GREATER 14)
    set(_default_archive_http ON)
endif()
option(CAQTDM_ENABLE_ARCHIVE_SF_PLUGIN "Build the archiveSF plugin" ${_default_archive_sf})
option(CAQTDM_ENABLE_ARCHIVE_HTTP_PLUGIN "Build the archiveHTTP plugin (requires Qt >= 5.15)" ${_default_archive_http})

set(_is_64bit FALSE)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_is_64bit TRUE)
endif()
set(CAQTDM_IS_64BIT ${_is_64bit} CACHE BOOL "Is this a 64-bit build" FORCE)

if(CAQTDM_IS_64BIT)
    set(CAQTDM_ARCH_SUFFIX 64)
else()
    set(CAQTDM_ARCH_SUFFIX 32)
endif()
set(CAQTDM_ARCH_SUFFIX ${CAQTDM_ARCH_SUFFIX} CACHE STRING "Architecture-specific suffix for bundled PSI libraries" FORCE)

set(_logging_lib_name "libNewLogRPC.a")
if(CAQTDM_IS_64BIT)
    set(_logging_lib_name "libNewLogRPC_64.a")
endif()
if(CAQTDM_LOGGING_ARCHIVE_LIB_DIR)
    set(CAQTDM_LOGGING_ARCHIVE_LIBRARY "${CAQTDM_LOGGING_ARCHIVE_LIB_DIR}/${_logging_lib_name}")
else()
    set(CAQTDM_LOGGING_ARCHIVE_LIBRARY "")
endif()
if(CAQTDM_CA_ARCHIVE_LIB_DIR)
    set(CAQTDM_CA_STORAGE_LIBRARY "${CAQTDM_CA_ARCHIVE_LIB_DIR}/Storage/libStorage_${CAQTDM_ARCH_SUFFIX}.a")
    set(CAQTDM_CA_TOOLS_LIBRARY "${CAQTDM_CA_ARCHIVE_LIB_DIR}/Tools/libTools_${CAQTDM_ARCH_SUFFIX}.a")
    set(CAQTDM_CA_XERCES_LIBRARY "${CAQTDM_CA_ARCHIVE_LIB_DIR}/xerces-c-3.1.4/libxerces-c_${CAQTDM_ARCH_SUFFIX}.a")
else()
    set(CAQTDM_CA_STORAGE_LIBRARY "")
    set(CAQTDM_CA_TOOLS_LIBRARY "")
    set(CAQTDM_CA_XERCES_LIBRARY "")
endif()

if(CAQTDM_LOGGING_ARCHIVE_LIBRARY AND EXISTS "${CAQTDM_LOGGING_ARCHIVE_LIBRARY}")
    set(CAQTDM_HAS_LOGGING_ARCHIVE_LIBS TRUE)
else()
    set(CAQTDM_HAS_LOGGING_ARCHIVE_LIBS FALSE)
endif()

if(CAQTDM_CA_STORAGE_LIBRARY AND CAQTDM_CA_TOOLS_LIBRARY AND CAQTDM_CA_XERCES_LIBRARY
   AND EXISTS "${CAQTDM_CA_STORAGE_LIBRARY}"
   AND EXISTS "${CAQTDM_CA_TOOLS_LIBRARY}"
   AND EXISTS "${CAQTDM_CA_XERCES_LIBRARY}")
    set(CAQTDM_HAS_CA_ARCHIVE_LIBS TRUE)
else()
    set(CAQTDM_HAS_CA_ARCHIVE_LIBS FALSE)
endif()

option(CAQTDM_ENABLE_ARCHIVE_HIPA_PLUGIN "Build the archiveHIPA plugin (logging archive access)" ${CAQTDM_HAS_LOGGING_ARCHIVE_LIBS})
option(CAQTDM_ENABLE_ARCHIVE_PRO_PLUGIN "Build the archivePRO plugin (logging archive access)" ${CAQTDM_HAS_LOGGING_ARCHIVE_LIBS})
option(CAQTDM_ENABLE_ARCHIVE_CA_PLUGIN "Build the archiveCA plugin (Channel Access archive)" ${CAQTDM_HAS_CA_ARCHIVE_LIBS})

set(_default_epics4 OFF)
if(EXISTS "${CAQTDM_EPICS_INCLUDE_DIR}/pv/pvAccess.h")
    set(_default_epics4 ON)
endif()
# EPICS4 plugin has compiler flag conflicts on Windows with Ninja generator
# Disable by default on Windows+Ninja (works fine with Visual Studio generator)
if(WIN32 AND CMAKE_GENERATOR MATCHES "Ninja")
    set(_default_epics4 OFF)
endif()
option(CAQTDM_ENABLE_EPICS4_PLUGIN "Build the EPICS4/pvAccess control system plugin" ${_default_epics4})
if(CAQTDM_ENABLE_EPICS4_PLUGIN AND NOT EXISTS "${CAQTDM_EPICS_INCLUDE_DIR}/pv/pvAccess.h")
    message(WARNING "EPICS4 headers not found in ${CAQTDM_EPICS_INCLUDE_DIR}; disabling epics4 plugin")
    set(CAQTDM_ENABLE_EPICS4_PLUGIN OFF CACHE BOOL "Build the EPICS4/pvAccess control system plugin" FORCE)
endif()

if(CAQTDM_ENABLE_BSREAD_PLUGIN AND NOT EXISTS "${CAQTDM_ZMQ_INCLUDE_DIR}/zmq.h")
    message(WARNING "ZeroMQ header not found in ${CAQTDM_ZMQ_INCLUDE_DIR}; disabling bsread plugin")
    set(CAQTDM_ENABLE_BSREAD_PLUGIN OFF CACHE BOOL "Build the bsread control system plugin" FORCE)
endif()

if(CAQTDM_ENABLE_ARCHIVE_HIPA_PLUGIN AND NOT CAQTDM_HAS_LOGGING_ARCHIVE_LIBS)
    message(WARNING "Required logging archive libraries not present; disabling archiveHIPA plugin")
    set(CAQTDM_ENABLE_ARCHIVE_HIPA_PLUGIN OFF CACHE BOOL "Build the archiveHIPA plugin (logging archive access)" FORCE)
endif()
if(CAQTDM_ENABLE_ARCHIVE_PRO_PLUGIN AND NOT CAQTDM_HAS_LOGGING_ARCHIVE_LIBS)
    message(WARNING "Required logging archive libraries not present; disabling archivePRO plugin")
    set(CAQTDM_ENABLE_ARCHIVE_PRO_PLUGIN OFF CACHE BOOL "Build the archivePRO plugin (logging archive access)" FORCE)
endif()
if(CAQTDM_ENABLE_ARCHIVE_CA_PLUGIN AND NOT CAQTDM_HAS_CA_ARCHIVE_LIBS)
    message(WARNING "Required CA archive libraries not present; disabling archiveCA plugin")
    set(CAQTDM_ENABLE_ARCHIVE_CA_PLUGIN OFF CACHE BOOL "Build the archiveCA plugin (Channel Access archive)" FORCE)
endif()

find_library(CAQTDM_EPICS_LIB_CA
    NAMES ca libca
    HINTS ${CAQTDM_EPICS_LIBRARY_DIR})
find_library(CAQTDM_EPICS_LIB_COM
    NAMES Com libCom
    HINTS ${CAQTDM_EPICS_LIBRARY_DIR})
find_library(CAQTDM_EPICS_LIB_DB
    NAMES dbRecStd libdbRecStd
    HINTS ${CAQTDM_EPICS_LIBRARY_DIR})

if(NOT CAQTDM_EPICS_LIB_CA OR NOT CAQTDM_EPICS_LIB_COM)
    message(FATAL_ERROR "Could not locate EPICS base libraries (ca/Com) inside ${CAQTDM_EPICS_LIBRARY_DIR}. Set EPICSLIB accordingly.")
endif()
if(NOT CAQTDM_EPICS_LIB_DB)
    message(FATAL_ERROR "Could not locate EPICS dbRecStd library inside ${CAQTDM_EPICS_LIBRARY_DIR}. Set EPICSLIB accordingly.")
endif()

set(CAQTDM_EPICS_CORE_LIBRARIES "${CAQTDM_EPICS_LIB_CA};${CAQTDM_EPICS_LIB_COM};${CAQTDM_EPICS_LIB_DB}" CACHE STRING "Core EPICS libraries" FORCE)

if(CAQTDM_ENABLE_BSREAD_PLUGIN)
    set(_caqtdm_zmq_names)
    
    # Allow explicit override via environment variable or CMake variable
    if(CAQTDM_ZMQ_LIBRARY_NAME AND NOT "${CAQTDM_ZMQ_LIBRARY_NAME}" STREQUAL "")
        list(APPEND _caqtdm_zmq_names "${CAQTDM_ZMQ_LIBRARY_NAME}")
    endif()
    
    # Construct Windows-specific ZeroMQ library name based on compiler and version
    if(WIN32 AND MSVC)
        # Get ZMQ version from environment or default to 4_3_5
        if(DEFINED ENV{ZMQ_VERSION})
            set(_zmq_version "$ENV{ZMQ_VERSION}")
            # Convert dots to underscores (e.g., 4.3.5 -> 4_3_5)
            string(REPLACE "." "_" _zmq_version "${_zmq_version}")
        else()
            set(_zmq_version "4_3_5")
        endif()
        
        # Detect Visual Studio version
        if(MSVC_VERSION GREATER_EQUAL 1930)
            # Visual Studio 2022 (v143)
            set(_zmq_toolset "v143")
        elseif(MSVC_VERSION GREATER_EQUAL 1920)
            # Visual Studio 2019 (v142)
            set(_zmq_toolset "v142")
        elseif(MSVC_VERSION GREATER_EQUAL 1910)
            # Visual Studio 2017 (v141)
            set(_zmq_toolset "v141")
        else()
            # Default to v142
            set(_zmq_toolset "v142")
        endif()
        
        # Allow override with ZMQ_LIB_SUFFIX environment variable
        if(DEFINED ENV{ZMQ_LIB_SUFFIX})
            set(_zmq_constructed_name "libzmq-$ENV{ZMQ_LIB_SUFFIX}-${_zmq_version}")
        else()
            set(_zmq_constructed_name "libzmq-${_zmq_toolset}-mt-${_zmq_version}")
        endif()
        
        message(STATUS "Detected Visual Studio toolset: ${_zmq_toolset}")
        message(STATUS "Looking for ZeroMQ library: ${_zmq_constructed_name}.lib")
        
        list(APPEND _caqtdm_zmq_names "${_zmq_constructed_name}")
    endif()
    
    # Add generic fallback names
    list(APPEND _caqtdm_zmq_names zmq libzmq)
    
    find_library(CAQTDM_ZMQ_LIBRARY
        NAMES ${_caqtdm_zmq_names}
        HINTS ${CAQTDM_ZMQ_LIBRARY_DIR})
    if(NOT CAQTDM_ZMQ_LIBRARY)
        message(WARNING "ZeroMQ library not found in ${CAQTDM_ZMQ_LIBRARY_DIR}; disabling bsread plugin")
        set(CAQTDM_ENABLE_BSREAD_PLUGIN OFF CACHE BOOL "Build the bsread control system plugin" FORCE)
    else()
        message(STATUS "Found ZeroMQ library: ${CAQTDM_ZMQ_LIBRARY}")
    endif()
endif()

# Mirror the qmake dependency graph from all.pro using lightweight registrations.
caqtdm_register_subproject(NAME caQtDM_Parsers PATH caQtDM_Parsers)
caqtdm_register_subproject(NAME caQtDM_QtControls PATH caQtDM_QtControls DEPENDS caQtDM_Parsers)
caqtdm_register_subproject(NAME qtcontrols_controllers PATH caQtDM_QtControls/plugins/qtcontrols_controllers DEPENDS caQtDM_QtControls caQtDM_Lib)
caqtdm_register_subproject(NAME qtcontrols_graphics PATH caQtDM_QtControls/plugins/qtcontrols_graphics DEPENDS caQtDM_QtControls caQtDM_Lib)
caqtdm_register_subproject(NAME qtcontrols_monitors PATH caQtDM_QtControls/plugins/qtcontrols_monitors DEPENDS caQtDM_QtControls caQtDM_Lib)
caqtdm_register_subproject(NAME qtcontrols_utilities PATH caQtDM_QtControls/plugins/qtcontrols_utilities DEPENDS caQtDM_QtControls caQtDM_Lib)
caqtdm_register_subproject(NAME caQtDM_Plugins PATH caQtDM_Lib/caQtDM_Plugins DEPENDS caQtDM_Lib)
caqtdm_register_subproject(NAME caQtDM_AndroidFunctions PATH caQtDM_AndroidFunctions CONDITION ANDROID)

set(_caqtdm_lib_deps caQtDM_QtControls)
if(CAQTDM_ENABLE_ADL_EDL)
    list(APPEND _caqtdm_lib_deps caQtDM_Parsers)
endif()
if(ANDROID)
    list(APPEND _caqtdm_lib_deps caQtDM_AndroidFunctions)
endif()
caqtdm_register_subproject(NAME caQtDM_Lib PATH caQtDM_Lib DEPENDS ${_caqtdm_lib_deps})

set(_viewer_deps
    caQtDM_QtControls
    caQtDM_Lib
    qtcontrols_controllers
    qtcontrols_graphics
    qtcontrols_utilities
    qtcontrols_monitors
    caQtDM_Plugins)
caqtdm_register_subproject(NAME caQtDM_Viewer PATH caQtDM_Viewer DEPENDS ${_viewer_deps})

caqtdm_register_subproject(NAME parser PATH caQtDM_Viewer/parser CONDITION "NOT CAQTDM_ENABLE_MOBILE" DEPENDS caQtDM_Parsers)
caqtdm_register_subproject(NAME parserEDM PATH caQtDM_Viewer/parserEDM CONDITION "UNIX AND NOT CAQTDM_ENABLE_MOBILE" DEPENDS caQtDM_Parsers)

caqtdm_add_registered_subprojects()

message(STATUS "caQtDM ${PROJECT_VERSION} configured. Registered subprojects will be added once their CMakeLists.txt files are available.")
