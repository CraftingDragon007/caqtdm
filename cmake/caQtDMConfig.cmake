# Core caQtDM configuration: options, dependency detection, feature switches.
# Mirrors the qmake logic in caQtDM_Viewer/qtdefs.pri + caQtDM.pri (Qt6 only).

include(FeatureSummary)
include(GNUInstallDirs)

# --------------------------------------------------------------------------------------------------
# Options (feature switches, mirror the CAQTDM_* env switches of the qmake build)
# --------------------------------------------------------------------------------------------------
option(CAQTDM_BUILD_GPS     "Build the GPS control-system plugin"                      OFF)
option(CAQTDM_BUILD_MODBUS  "Build the Modbus control-system plugin"                   OFF)
option(CAQTDM_BUILD_OPCUA   "Build the OPC UA control-system plugin"                   OFF)
option(CAQTDM_ADL_EDL       "Enable on-the-fly ADL/EDL (.adl/.edl) conversion"         ON)
option(CAQTDM_PYTHONCALC    "Python support for caCalc/visibility calculations"        ON)
option(CAQTDM_WEB           "Build caQtDM Web support (websocket server)"              OFF)
option(CAQTDM_WITH_TESTS    "Build unit tests and register them with CTest"            ON)
option(CAQTDM_NORPATH       "Build without rpath entries (packaging builds)"           OFF)
option(CAQTDM_NO_CUSTOM_LOGHANDLER "Build the viewer without the custom log handlers"  OFF)

# --------------------------------------------------------------------------------------------------
# Path configuration
# --------------------------------------------------------------------------------------------------
set(CAQTDM_COLLECT "${CMAKE_BINARY_DIR}/caQtDM_Binaries" CACHE PATH
    "Collect directory all libraries/plugins/executables are built into")

set(CAQTDM_EPICS_BASE "" CACHE PATH "EPICS base directory")
set(CAQTDM_EPICS_HOST_ARCH "" CACHE STRING
    "EPICS host architecture (e.g. linux-x86_64); derived from the platform when empty")

set(CAQTDM_QWT_HOME "" CACHE PATH "Qwt installation prefix")
set(CAQTDM_QWT_INCLUDE "" CACHE PATH "Qwt include directory")
set(CAQTDM_QWT_LIB "" CACHE PATH "Qwt library directory")
set(CAQTDM_QWT_LIBNAME "qwt" CACHE STRING "Qwt library name (qwt, qwt-qt5, qwt-qt6, ...)")

set(CAQTDM_ZMQ_INCLUDE "" CACHE PATH "ZeroMQ include directory")
set(CAQTDM_ZMQ_LIB "" CACHE PATH "ZeroMQ library directory")

set(CAQTDM_PYTHON_ROOT "" CACHE PATH "Python installation prefix used for PYTHONCALC")
set(CAQTDM_PYTHON_INCLUDE "" CACHE PATH "Python include directory override for PYTHONCALC")
set(CAQTDM_PYTHON_LIBRARY "" CACHE FILEPATH "Python library file override for PYTHONCALC")

# --------------------------------------------------------------------------------------------------
# Mobile detection (mirrors MOBILE in qtdefs.pri)
# --------------------------------------------------------------------------------------------------
if(ANDROID OR IOS)
    set(CAQTDM_MOBILE ON)
else()
    set(CAQTDM_MOBILE OFF)
endif()

# --------------------------------------------------------------------------------------------------
# Version string with git suffix (mirrors qtdefs.pri)
# --------------------------------------------------------------------------------------------------
set(CAQTDM_VERSION_STR "V${PROJECT_VERSION}")
find_package(Git QUIET)
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE _caqtdm_git_branch OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
if(_caqtdm_git_branch MATCHES "Development")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 origin/${_caqtdm_git_branch}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE _caqtdm_git_hash OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(NOT _caqtdm_git_hash STREQUAL "")
        string(APPEND CAQTDM_VERSION_STR "_${_caqtdm_git_branch}_${_caqtdm_git_hash}")
    endif()
endif()
message(STATUS "caQtDM version: ${CAQTDM_VERSION_STR}")

# --------------------------------------------------------------------------------------------------
# Qt6
# --------------------------------------------------------------------------------------------------
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Gui Widgets Network Xml OpenGL Concurrent UiTools PrintSupport Svg Designer Test)
find_package(Qt6 QUIET COMPONENTS Positioning SerialBus OpcUa WebSockets)

if(MSVC)
    # The EPICS pvAccess headers need /Zc:twoPhase- to compile under the
    # -permissive- conformance mode that the Qt6 kits enable. The option
    # must appear AFTER -permissive- on the command line, so it is appended
    # to the interface options of Qt6::Platform instead of the targets.
    set_property(TARGET Qt6::Platform APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
        "/Zc:twoPhase-;-Zc:referenceBinding")
endif()

if(CAQTDM_BUILD_GPS AND NOT TARGET Qt6::Positioning)
    message(FATAL_ERROR "CAQTDM_BUILD_GPS requires the Qt6 Positioning module")
endif()
if(CAQTDM_BUILD_MODBUS AND NOT TARGET Qt6::SerialBus)
    message(FATAL_ERROR "CAQTDM_BUILD_MODBUS requires the Qt6 SerialBus module")
endif()
set(CAQTDM_HAVE_OPCUA OFF)
if(CAQTDM_BUILD_OPCUA)
    if(NOT TARGET Qt6::OpcUa)
        message(FATAL_ERROR "CAQTDM_BUILD_OPCUA requires the Qt6 OpcUa module")
    endif()
    set(CAQTDM_HAVE_OPCUA ON)
    get_target_property(_qt_install_headers Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
    find_path(QT_OPCUA_X509_HEADER QtOpcUa/QOpcUaX509CertificateSigningRequest
        HINTS ${_qt_install_headers} ${Qt6OpcUa_DIR}/../../include
        NO_DEFAULT_PATH)
    if(QT_OPCUA_X509_HEADER)
        set(CAQTDM_QT_OPCUA_X509 ON)
        message(STATUS "OPC UA plugin: encryption support enabled (QOpcUaX509 found)")
    else()
        set(CAQTDM_QT_OPCUA_X509 OFF)
        message(STATUS "OPC UA plugin: no QOpcUaX509 headers available, skipping encryption")
    endif()
endif()

set(CAQTDM_HAVE_WEBSOCKETS OFF)
if(CAQTDM_WEB)
    if(NOT TARGET Qt6::WebSockets)
        message(FATAL_ERROR "CAQTDM_WEB requires the Qt6 WebSockets module")
    endif()
    set(CAQTDM_HAVE_WEBSOCKETS ON)
endif()

# --------------------------------------------------------------------------------------------------
# Qwt / EPICS / ZeroMQ / Python
# --------------------------------------------------------------------------------------------------
find_package(Qwt REQUIRED)
find_package(Epics REQUIRED)

set(CAQTDM_HAVE_BSREAD OFF)
set(CAQTDM_ZMQ_LIB_DIR "")
if(NOT CAQTDM_MOBILE)
    find_path(ZMQ_INCLUDE_DIR zmq.h
        HINTS ${CAQTDM_ZMQ_INCLUDE}
        PATHS /usr/include /usr/local/include)
    if(ZMQ_INCLUDE_DIR)
        find_library(ZMQ_LIBRARY NAMES zmq
            HINTS ${CAQTDM_ZMQ_LIB}
            PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
        if(ZMQ_LIBRARY)
            set(CAQTDM_HAVE_BSREAD ON)
            add_library(caqtdm::zmq UNKNOWN IMPORTED)
            set_target_properties(caqtdm::zmq PROPERTIES
                IMPORTED_LOCATION "${ZMQ_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${ZMQ_INCLUDE_DIR}")
            if(CAQTDM_ZMQ_LIB)
                get_filename_component(CAQTDM_ZMQ_LIB_DIR "${CAQTDM_ZMQ_LIB}" ABSOLUTE)
            else()
                get_filename_component(CAQTDM_ZMQ_LIB_DIR "${ZMQ_LIBRARY}" DIRECTORY)
            endif()
        endif()
    endif()
endif()

set(CAQTDM_HAVE_PYTHON OFF)
if(CAQTDM_PYTHONCALC AND NOT CAQTDM_MOBILE)
    if(CAQTDM_PYTHON_INCLUDE AND CAQTDM_PYTHON_LIBRARY)
        add_library(caqtdm::python UNKNOWN IMPORTED)
        set_target_properties(caqtdm::python PROPERTIES
            IMPORTED_LOCATION "${CAQTDM_PYTHON_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CAQTDM_PYTHON_INCLUDE}")
        set(CAQTDM_HAVE_PYTHON ON)
    else()
        if(CAQTDM_PYTHON_ROOT)
            list(PREPEND CMAKE_PREFIX_PATH "${CAQTDM_PYTHON_ROOT}")
        endif()
        find_program(CAQTDM_PYTHON_INTERPRETER NAMES python3 python
            HINTS "${CAQTDM_PYTHON_ROOT}/bin")
        if(CAQTDM_PYTHON_INTERPRETER)
            execute_process(COMMAND ${CAQTDM_PYTHON_INTERPRETER} -c
                "import sysconfig; print(sysconfig.get_paths()['include'])"
                OUTPUT_VARIABLE _py_include OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            execute_process(COMMAND ${CAQTDM_PYTHON_INTERPRETER} -c
                "import sysconfig; v = sysconfig.get_config_var('LDVERSION') or '{}.{}'.format(*sys.version_info[:2]); print('python' + v.replace('.', '') if sys.platform == 'win32' else 'python' + v)"
                OUTPUT_VARIABLE _py_ldversion OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            execute_process(COMMAND ${CAQTDM_PYTHON_INTERPRETER} -c
                "import sysconfig, os; print(os.path.join(os.path.dirname(sys.executable), 'libs'))"
                OUTPUT_VARIABLE _py_libdir OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            find_library(CAQTDM_PYTHON_LIBFILE NAMES ${_py_ldversion} HINTS ${_py_libdir})
            if(CAQTDM_PYTHON_LIBFILE)
                add_library(caqtdm::python UNKNOWN IMPORTED)
                set_target_properties(caqtdm::python PROPERTIES
                    IMPORTED_LOCATION "${CAQTDM_PYTHON_LIBFILE}"
                    INTERFACE_INCLUDE_DIRECTORIES "${_py_include}")
                set(CAQTDM_HAVE_PYTHON ON)
            endif()
        endif()
    endif()
endif()
if(CAQTDM_PYTHONCALC AND NOT CAQTDM_HAVE_PYTHON)
    message(WARNING "PYTHONCALC is enabled but no Python development files were found - building without Python support")
endif()

# --------------------------------------------------------------------------------------------------
# Feature resolution (mirrors qtdefs.pri auto-detection)
# --------------------------------------------------------------------------------------------------
set(CAQTDM_EPICS7 OFF)
if(EXISTS "${Epics_INCLUDE_DIR}/pv/pvAccess.h")
    set(CAQTDM_EPICS7 ON)
endif()

if(CAQTDM_MOBILE)
    set(CAQTDM_ADL_EDL OFF)
    set(CAQTDM_ARCHIVE_PLUGINS OFF)
else()
    set(CAQTDM_ARCHIVE_PLUGINS ON)
endif()

add_feature_info(gps CAQTDM_BUILD_GPS "GPS control-system plugin")
add_feature_info(modbus CAQTDM_BUILD_MODBUS "Modbus control-system plugin")
add_feature_info(opcua CAQTDM_HAVE_OPCUA "OPC UA control-system plugin")
add_feature_info(bsread CAQTDM_HAVE_BSREAD "bsread control-system plugin (ZeroMQ autodetected)")
add_feature_info(epics7 CAQTDM_EPICS7 "EPICS 7 pvAccess support (epics4 plugin)")
add_feature_info(adl_edl CAQTDM_ADL_EDL "on-the-fly ADL/EDL conversion")
add_feature_info(pythoncalc CAQTDM_HAVE_PYTHON "Python support for caCalc/visibility")
add_feature_info(web CAQTDM_HAVE_WEBSOCKETS "caQtDM Web websocket server support")
add_feature_info(tests CAQTDM_WITH_TESTS "unit tests")

# --------------------------------------------------------------------------------------------------
# Global compile definitions (mirror the unconditional DEFINES in qtdefs.pri for Qt6)
# --------------------------------------------------------------------------------------------------
add_compile_definitions(
    XDR_HACK
    XDR_LE
    IO_OPTIMIZED_FOR_TABWIDGETS
    QT_MESSAGELOGCONTEXT
    "TARGET_COMPANY=\"Paul Scherrer Institut\""
    "TARGET_DESCRIPTION=\"Channel Access Qt Display Manager\""
    "TARGET_COPYRIGHT=\"Copyright (C) 2012-2025 Paul Scherrer Institut\""
    "TARGET_INTERNALNAME=\"caqtdm\""
    "TARGET_VERSION_STR=\"${CAQTDM_VERSION_STR}\""
    TARGET_VER_MAJ=${PROJECT_VERSION_MAJOR}
    TARGET_VER_MIN=${PROJECT_VERSION_MINOR}
    TARGET_VER_BUILD=${PROJECT_VERSION_PATCH}
)
if(CAQTDM_ADL_EDL)
    add_compile_definitions(ADL_EDL_FILES)
endif()
if(CAQTDM_MOBILE)
    add_compile_definitions(MOBILE)
    if(IOS)
        add_compile_definitions(MOBILE_IOS)
    endif()
    if(ANDROID)
        add_compile_definitions(MOBILE_ANDROID)
    endif()
endif()

# --------------------------------------------------------------------------------------------------
# Global build settings
# --------------------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
endif()

if(CAQTDM_NORPATH)
    set(CMAKE_SKIP_BUILD_RPATH TRUE)
    set(CMAKE_SKIP_INSTALL_RPATH TRUE)
endif()

if(WIN32)
    set(_caqtdm_cfgdir "$<$<CONFIG:Debug>:debug/>")
else()
    set(_caqtdm_cfgdir "")
endif()
set(CAQTDM_COLLECT_GENEX "${CAQTDM_COLLECT}/${_caqtdm_cfgdir}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CAQTDM_COLLECT_GENEX}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CAQTDM_COLLECT_GENEX}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CAQTDM_COLLECT_GENEX}")

# --------------------------------------------------------------------------------------------------
# Feature summary
# --------------------------------------------------------------------------------------------------
feature_summary(WHAT ENABLED_FEATURES DISABLED_FEATURES
    DESCRIPTION "--- caQtDM feature summary ---")


