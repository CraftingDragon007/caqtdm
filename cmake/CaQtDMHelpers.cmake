# SPDX-License-Identifier: GPL-3.0-or-later
# Helper utilities shared by the caQtDM CMake build.

include_guard(GLOBAL)

function(caqtdm_require_path_from_env out_var env_var description)
    if(NOT description)
        set(description "${env_var} path")
    endif()

    if(NOT DEFINED ${out_var} OR "${${out_var}}" STREQUAL "")
        if(DEFINED ENV{${env_var}})
            set(${out_var} "$ENV{${env_var}}")
        else()
            set(${out_var} "")
        endif()
    endif()

    set(${out_var} "${${out_var}}" CACHE PATH "${description}")

    if("${${out_var}}" STREQUAL "")
        message(FATAL_ERROR "${env_var} must be defined (${description}).")
    endif()

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
    message(STATUS "${env_var} located at ${${out_var}}")
endfunction()

function(caqtdm_require_string_from_env out_var env_var description)
    if(NOT description)
        set(description "${env_var} value")
    endif()

    if(NOT DEFINED ${out_var} OR "${${out_var}}" STREQUAL "")
        if(DEFINED ENV{${env_var}})
            set(${out_var} "$ENV{${env_var}}")
        else()
            set(${out_var} "")
        endif()
    endif()

    set(${out_var} "${${out_var}}" CACHE STRING "${description}")

    if("${${out_var}}" STREQUAL "")
        message(FATAL_ERROR "${env_var} must be defined (${description}).")
    endif()

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
    message(STATUS "${env_var} value: ${${out_var}}")
endfunction()

function(caqtdm_bool_from_env out_var env_var default)
    set(value "${default}")
    if(DEFINED ENV{${env_var}})
        string(TOLOWER "$ENV{${env_var}}" env_value)
        if(env_value MATCHES "^(1|on|true|yes)$")
            set(value ON)
        elseif(env_value MATCHES "^(0|off|false|no)$")
            set(value OFF)
        else()
            set(value "${default}")
        endif()
    endif()
    set(${out_var} ${value} PARENT_SCOPE)
endfunction()

function(caqtdm_cache_path_from_env out_var env_var description)
    set(fallback "")
    if(ARGC GREATER 3)
        set(fallback "${ARGV3}")
    endif()

    if(DEFINED ${out_var} AND NOT "${${out_var}}" STREQUAL "")
        set(value "${${out_var}}")
    elseif(DEFINED ENV{${env_var}} AND NOT "$ENV{${env_var}}" STREQUAL "")
        set(value "$ENV{${env_var}}")
    else()
        set(value "${fallback}")
    endif()

    set(${out_var} "${value}" CACHE PATH "${description}")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(caqtdm_cache_string_from_env out_var env_var description)
    set(fallback "")
    if(ARGC GREATER 3)
        set(fallback "${ARGV3}")
    endif()

    if(DEFINED ${out_var} AND NOT "${${out_var}}" STREQUAL "")
        set(value "${${out_var}}")
    elseif(DEFINED ENV{${env_var}} AND NOT "$ENV{${env_var}}" STREQUAL "")
        set(value "$ENV{${env_var}}")
    else()
        set(value "${fallback}")
    endif()

    set(${out_var} "${value}" CACHE STRING "${description}")
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(caqtdm_target_version_info target)
    set(options)
    set(oneValueArgs PRODUCT FILENAME DESCRIPTION INTERNAL_NAME)
    cmake_parse_arguments(META "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT TARGET ${target})
        message(FATAL_ERROR "caqtdm_target_version_info: target '${target}' does not exist")
    endif()
    if(NOT META_PRODUCT)
        message(FATAL_ERROR "caqtdm_target_version_info requires PRODUCT for target ${target}")
    endif()
    if(NOT META_FILENAME)
        message(FATAL_ERROR "caqtdm_target_version_info requires FILENAME for target ${target}")
    endif()

    if(NOT META_DESCRIPTION)
        set(META_DESCRIPTION "${CAQTDM_TARGET_DESCRIPTION}")
    endif()
    if(NOT META_INTERNAL_NAME)
        set(META_INTERNAL_NAME "${CAQTDM_TARGET_INTERNAL_NAME}")
    endif()

    target_compile_definitions(${target} PRIVATE
        "TARGET_PRODUCT=\"${META_PRODUCT}\""
        "TARGET_FILENAME=\"${META_FILENAME}\""
        "TARGET_DESCRIPTION=\"${META_DESCRIPTION}\""
        "TARGET_COMPANY=\"${CAQTDM_TARGET_COMPANY}\""
        "TARGET_COPYRIGHT=\"${CAQTDM_TARGET_COPYRIGHT}\""
        "TARGET_INTERNALNAME=\"${META_INTERNAL_NAME}\""
        "TARGET_VERSION_STR=\"${CAQTDM_VERSION_STR}\""
        TARGET_VER_MAJ=${CAQTDM_VERSION_MAJOR}
        TARGET_VER_MIN=${CAQTDM_VERSION_MINOR}
        TARGET_VER_BUILD=${CAQTDM_VERSION_PATCH})
endfunction()

function(caqtdm_target_build_info target)
    set(options)
    set(oneValueArgs SUPPORT)
    cmake_parse_arguments(BUILD "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT TARGET ${target})
        message(FATAL_ERROR "caqtdm_target_build_info: target '${target}' does not exist")
    endif()

    target_compile_definitions(${target} PRIVATE
        "BUILDVERSION=\"${CAQTDM_VERSION_STR}\""
        "BUILDARCH=\"${CAQTDM_BUILD_ARCH}\""
        "BUILDTIME=\"${CAQTDM_BUILD_TIME}\""
        "BUILDDATE=\"${CAQTDM_BUILD_DATE}\"")

    if(BUILD_SUPPORT)
        target_compile_definitions(${target} PRIVATE "SUPPORT=\"${BUILD_SUPPORT}\"")
    endif()
endfunction()
