# Shared helper functions for the caQtDM CMake build.

# Places a target's outputs into the collect directory (optionally in a
# subdirectory like "controlsystems" or "designer"). On multi-config
# generators (Visual Studio) Debug builds go into a debug/ subdirectory,
# mirroring the qmake DESTDIR layout on Windows.
function(caqtdm_set_output_dir target)
    cmake_parse_arguments(ARG "" "SUBDIR" "" ${ARGN})
    set(_dir "${CAQTDM_COLLECT_GENEX}")
    if(ARG_SUBDIR)
        string(APPEND _dir "${ARG_SUBDIR}/")
    endif()
    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${_dir}"
        RUNTIME_OUTPUT_DIRECTORY "${_dir}"
        ARCHIVE_OUTPUT_DIRECTORY "${_dir}")
endfunction()

# Adds rpath entries so the binaries find their libraries without
# LD_LIBRARY_PATH, unless CAQTDM_NORPATH is set (packaging builds).
function(caqtdm_apply_rpath target)
    if(CAQTDM_NORPATH OR NOT UNIX)
        return()
    endif()
    if(APPLE)
        set(_origin "@loader_path")
        set(_bundle_rpaths "@loader_path/../Frameworks" "@loader_path/../../Frameworks")
    else()
        set(_origin "\$ORIGIN")
        set(_bundle_rpaths "")
    endif()
    set(_rpaths "${_origin}" "${_origin}/controlsystems" "${_origin}/designer")
    list(APPEND _rpaths ${_bundle_rpaths})
    list(APPEND _rpaths "${Epics_LIBRARY_DIR}" "${Qwt_LIBRARY_DIR}")
    if(CAQTDM_ZMQ_LIB_DIR)
        list(APPEND _rpaths "${CAQTDM_ZMQ_LIB_DIR}")
    endif()
    if(CAQTDM_PYTHON_LIBRARY)
        get_filename_component(_pydir "${CAQTDM_PYTHON_LIBRARY}" DIRECTORY)
        list(APPEND _rpaths "${_pydir}")
    elseif(TARGET Python3::Module)
        list(APPEND _rpaths "${Python3_LIBRARY_DIRS}")
    endif()
    if(target STREQUAL "caQtDM")
        set(_install_lib_path "${_caqtdm_install_libdir}")
    elseif(target MATCHES "(_plugin|_Plugin)$")
        if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
            set(_install_lib_path "${CMAKE_INSTALL_LIBDIR}")
        else()
            set(_install_lib_path "..")
        endif()
    elseif(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
        set(_install_lib_path "${CMAKE_INSTALL_LIBDIR}")
    else()
        set(_install_lib_path ".")
    endif()
    set(_install_rpaths "${_origin}")
    if(IS_ABSOLUTE "${_install_lib_path}")
        list(APPEND _install_rpaths "${_install_lib_path}")
    else()
        list(APPEND _install_rpaths "${_origin}/${_install_lib_path}")
    endif()
    list(APPEND _install_rpaths ${_rpaths} ${_bundle_rpaths})
    set_target_properties(${target} PROPERTIES
        BUILD_RPATH "${_rpaths}"
        INSTALL_RPATH "${_install_rpaths}")
    if(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
        target_link_options(${target} PRIVATE "LINKER:-z,origin")
    endif()
endfunction()

# Common include directories shared by everything that consumes the
# widget library / dispatcher library headers.
function(caqtdm_common_includes target)
    target_include_directories(${target} ${ARGN}
        "${PROJECT_SOURCE_DIR}/caQtDM_Plugins"
        "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src"
        "${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/prcParserSrc")
endfunction()

# Per-target version info resource for Windows (mirrors the per-project
# TARGET_PRODUCT / TARGET_FILENAME values of the qmake .pro files).
# The .rc compiler does not receive target compile definitions, so each
# target gets its own copy of the .rc with the definitions attached as
# source-file compile definitions.
function(caqtdm_add_version_rc target rcfile product filename)
    if(NOT WIN32)
        return()
    endif()
    set(_rc_copy "${CMAKE_CURRENT_BINARY_DIR}/${target}_version.rc")
    configure_file("${rcfile}" "${_rc_copy}" COPYONLY)
    cmake_path(GET rcfile PARENT_PATH _rc_dir)
    file(GLOB _rc_icons LIST_DIRECTORIES false "${_rc_dir}/*.ico")
    foreach(_icon ${_rc_icons})
        configure_file("${_icon}" "${CMAKE_CURRENT_BINARY_DIR}/" COPYONLY)
    endforeach()
    set_source_files_properties("${_rc_copy}" PROPERTIES
        COMPILE_DEFINITIONS "TARGET_VER_MAJ=${PROJECT_VERSION_MAJOR};TARGET_VER_MIN=${PROJECT_VERSION_MINOR};TARGET_VER_BUILD=${PROJECT_VERSION_PATCH};TARGET_COMPANY=\"Paul Scherrer Institut\";TARGET_DESCRIPTION=\"Channel Access Qt Display Manager\";TARGET_COPYRIGHT=\"Copyright (C) 2012-2025 Paul Scherrer Institut\";TARGET_INTERNALNAME=\"caqtdm\";TARGET_VERSION_STR=\"${CAQTDM_VERSION_STR}\";TARGET_PRODUCT=\"${product}\";TARGET_FILENAME=\"${filename}\"")
    target_sources(${target} PRIVATE "${_rc_copy}")
endfunction()

# Creates one control-system plugin below <collect>/controlsystems.
# A MODULE on desktop platforms, except linkable Windows plugins which
# use SHARED to produce an import library; a STATIC library on mobile
# (mirrors CONFIG += staticlib in caQtDM.pri). All plugins link against caQtDM_Lib.
function(caqtdm_add_cs_plugin name)
    cmake_parse_arguments(ARG "LINKABLE" "CLASS_NAME" "SOURCES;HEADERS;FORMS;LINKS;DEFINES;INCLUDES" ${ARGN})
    if(CAQTDM_MOBILE)
        if(NOT ARG_CLASS_NAME)
            message(FATAL_ERROR "Mobile plugin ${name} needs CLASS_NAME for Qt static registration")
        endif()
        qt_add_plugin(${name} STATIC CLASS_NAME "${ARG_CLASS_NAME}" ${ARG_SOURCES} ${ARG_HEADERS})
    else()
        if(WIN32 AND ARG_LINKABLE)
            # The bsread unit test links against the plugin's import library.
            add_library(${name} SHARED ${ARG_SOURCES} ${ARG_HEADERS})
        else()
            add_library(${name} MODULE ${ARG_SOURCES} ${ARG_HEADERS})
        endif()
        if(APPLE)
            set_target_properties(${name} PROPERTIES SUFFIX ".dylib")
        endif()
    endif()
    set_property(GLOBAL APPEND PROPERTY CAQTDM_CONTROL_SYSTEM_PLUGIN_TARGETS ${name})
    if(ARG_FORMS)
        target_sources(${name} PRIVATE ${ARG_FORMS})
    endif()
    target_include_directories(${name} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/caQtDM_Plugins"
        "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src"
        "${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src"
        "${Epics_INCLUDE_DIR}"
        ${Epics_INCLUDE_DIRS}
        ${ARG_INCLUDES})
    target_link_libraries(${name} PRIVATE caQtDM_Lib ${ARG_LINKS})
    if(ARG_DEFINES)
        target_compile_definitions(${name} PRIVATE ${ARG_DEFINES})
    endif()
    if(MSVC)
        target_compile_definitions(${name} PRIVATE CAQTDM_PLUGIN_LIBRARY _CRT_SECURE_NO_WARNINGS)
    endif()
    caqtdm_set_output_dir(${name} SUBDIR controlsystems)
    caqtdm_apply_rpath(${name})
    if(NOT CAQTDM_MOBILE)
        install(TARGETS ${name}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/controlsystems
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/controlsystems)
    endif()
endfunction()
