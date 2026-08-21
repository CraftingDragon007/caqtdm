# FindEpics
# ---------
# Locates EPICS base libraries and creates imported targets:
#   Epics::ca Epics::Com Epics::pvAccess Epics::pvAccessCA
#   Epics::pvData Epics::pvaClient Epics::nt
# Targets are only created for libraries that exist; consumers must check
# with `if(TARGET Epics::xxx)` for the optional pvAccess family.
#
# Inputs (set by caQtDMConfig.cmake):
#   CAQTDM_EPICS_BASE       EPICS base directory (required)
#   CAQTDM_EPICS_HOST_ARCH  host architecture, derived when empty
#
# Result variables:
#   Epics_INCLUDE_DIR, Epics_LIBRARY_DIR

if(NOT CAQTDM_EPICS_BASE)
    message(FATAL_ERROR "CAQTDM_EPICS_BASE is not set - point it at your EPICS base directory")
endif()

if(NOT CAQTDM_EPICS_HOST_ARCH)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
            set(CAQTDM_EPICS_HOST_ARCH "linux-aarch64")
        else()
            set(CAQTDM_EPICS_HOST_ARCH "linux-x86_64")
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(CAQTDM_EPICS_HOST_ARCH "darwin-x86_64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(CAQTDM_EPICS_HOST_ARCH "windows-x64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
        set(CAQTDM_EPICS_HOST_ARCH "android-arm64-v8a")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CAQTDM_EPICS_HOST_ARCH "ios-arm64")
    else()
        message(FATAL_ERROR "Cannot derive CAQTDM_EPICS_HOST_ARCH for ${CMAKE_SYSTEM_NAME} - set it explicitly")
    endif()
    message(STATUS "EPICS host architecture derived: ${CAQTDM_EPICS_HOST_ARCH}")
endif()

set(Epics_INCLUDE_DIR "${CAQTDM_EPICS_BASE}/include")
set(Epics_LIBRARY_DIR "${CAQTDM_EPICS_BASE}/lib/${CAQTDM_EPICS_HOST_ARCH}")

if(NOT EXISTS "${Epics_INCLUDE_DIR}")
    message(FATAL_ERROR "EPICS include directory not found: ${Epics_INCLUDE_DIR}")
endif()

# OS/compiler specific include subdirectories (mirrors caQtDM.pri)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(_epics_os_dir os/Linux)
    set(_epics_compiler_dir compiler/gcc)
elseif(APPLE)
    set(_epics_os_dir os/Darwin)
    set(_epics_compiler_dir compiler/clang)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(_epics_os_dir os/win32)
    set(_epics_compiler_dir compiler/msvc)
elseif(IOS)
    set(_epics_os_dir os/iOS)
    set(_epics_compiler_dir compiler/clang)
endif()
if(_epics_os_dir AND EXISTS "${Epics_INCLUDE_DIR}/${_epics_os_dir}")
    list(APPEND Epics_INCLUDE_DIRS "${Epics_INCLUDE_DIR}/${_epics_os_dir}")
endif()
if(_epics_compiler_dir AND EXISTS "${Epics_INCLUDE_DIR}/${_epics_compiler_dir}")
    list(APPEND Epics_INCLUDE_DIRS "${Epics_INCLUDE_DIR}/${_epics_compiler_dir}")
endif()

function(_epics_make_target name libname)
    find_library(_epics_${name}_LIBRARY NAMES ${libname}
        HINTS ${Epics_LIBRARY_DIR}
        NO_DEFAULT_PATH)
    if(_epics_${name}_LIBRARY)
        add_library(Epics::${name} UNKNOWN IMPORTED)
        set_target_properties(Epics::${name} PROPERTIES
            IMPORTED_LOCATION "${_epics_${name}_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Epics_INCLUDE_DIR};${Epics_INCLUDE_DIRS}")
    endif()
    mark_as_advanced(_epics_${name}_LIBRARY)
endfunction()

_epics_make_target(ca ca)
_epics_make_target(Com Com)
_epics_make_target(pvAccess pvAccess)
_epics_make_target(pvAccessCA pvAccessCA)
_epics_make_target(pvData pvData)
_epics_make_target(pvaClient pvaClient)
_epics_make_target(nt nt)

if(NOT TARGET Epics::ca OR NOT TARGET Epics::Com)
    message(FATAL_ERROR "EPICS ca/Com libraries not found in ${Epics_LIBRARY_DIR}")
endif()

set(Epics_FOUND TRUE)
message(STATUS "Found EPICS base ${CAQTDM_EPICS_BASE} (${CAQTDM_EPICS_HOST_ARCH})")
