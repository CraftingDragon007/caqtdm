# SPDX-License-Identifier: GPL-3.0-or-later
# Helpers to declare caQtDM control system plugins in CMake.

include_guard(GLOBAL)

function(caqtdm_add_plugin target)
    set(options)
    set(oneValueArgs OUTPUT_NAME)
    set(multiValueArgs SOURCES QT_COMPONENTS EXTRA_LIBS EXTRA_INCLUDES DEFINES)
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT PLUGIN_SOURCES)
        message(FATAL_ERROR "caqtdm_add_plugin(${target}) requires SOURCES")
    endif()

    set(_library_type MODULE)
    if(IOS OR ANDROID)
        set(_library_type STATIC)
    endif()

    add_library(${target} ${_library_type} ${PLUGIN_SOURCES})

    if(PLUGIN_OUTPUT_NAME)
        set_target_properties(${target} PROPERTIES OUTPUT_NAME ${PLUGIN_OUTPUT_NAME})
    endif()

    set(_plugin_includes
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
        ${PROJECT_SOURCE_DIR}/caQtDM_Lib/caQtDM_Plugins
        ${PROJECT_SOURCE_DIR}/caQtDM_Lib/src
        ${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src
        ${CAQTDM_QWT_INCLUDE_DIR}
        ${PLUGIN_EXTRA_INCLUDES})

    if(CAQTDM_EPICS_INCLUDE_DIR)
        list(APPEND _plugin_includes ${CAQTDM_EPICS_INCLUDE_DIR})
        
        # Add platform-specific EPICS include directories
        if(WIN32)
            list(APPEND _plugin_includes ${CAQTDM_EPICS_INCLUDE_DIR}/os/win32)
            if(MSVC)
                list(APPEND _plugin_includes ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/msvc)
            elseif(MINGW)
                list(APPEND _plugin_includes ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/gcc)
            endif()
        elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
            list(APPEND _plugin_includes
                ${CAQTDM_EPICS_INCLUDE_DIR}/os/freebsd
                ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/clang)
        elseif(APPLE)
            list(APPEND _plugin_includes
                ${CAQTDM_EPICS_INCLUDE_DIR}/os/Darwin
                ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/clang
                ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/gcc)
        elseif(UNIX)
            list(APPEND _plugin_includes
                ${CAQTDM_EPICS_INCLUDE_DIR}/os/Linux
                ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/gcc)
        endif()
    endif()

    if(CAQTDM_ANDROID_FUNCTIONS_INCLUDE)
        list(APPEND _plugin_includes ${CAQTDM_ANDROID_FUNCTIONS_INCLUDE})
    endif()

    list(REMOVE_DUPLICATES _plugin_includes)
    target_include_directories(${target} PRIVATE ${_plugin_includes})

    set(_qt_components Core Gui Widgets)
    if(PLUGIN_QT_COMPONENTS)
        list(APPEND _qt_components ${PLUGIN_QT_COMPONENTS})
    endif()
    list(REMOVE_DUPLICATES _qt_components)

    foreach(component IN LISTS _qt_components)
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${component})
        target_link_libraries(${target} PRIVATE Qt${QT_VERSION_MAJOR}::${component})
    endforeach()

    target_link_libraries(${target} PRIVATE caqtdm_lib qtcontrols ${PLUGIN_EXTRA_LIBS})
    
    # On Windows, plugins need direct access to EPICS libraries for calc functions (postfix, calcPerform)
    # Even though caQtDM_Lib links to EPICS, Windows DLL model doesn't support transitive symbol access
    if(CAQTDM_EPICS_CORE_LIBRARIES)
        target_link_libraries(${target} PRIVATE ${CAQTDM_EPICS_CORE_LIBRARIES})
    endif()

    if(CAQTDM_QT_CORE5COMPAT_TARGET)
        target_link_libraries(${target} PRIVATE ${CAQTDM_QT_CORE5COMPAT_TARGET})
    endif()

    if(PLUGIN_DEFINES)
        target_compile_definitions(${target} PRIVATE ${PLUGIN_DEFINES})
    endif()

    if(CAQTDM_ENABLE_RPATH AND CAQTDM_EPICS_LIBRARY_DIR)
        set_target_properties(${target} PROPERTIES BUILD_RPATH "${CAQTDM_EPICS_LIBRARY_DIR}")
    endif()

    # Install runtime plugins (shared libraries) when not building for mobile
    if(NOT ANDROID AND NOT IOS)
        install(TARGETS ${target}
            RUNTIME DESTINATION "${CAQTDM_INSTALL_CONTROL_PLUGINDIR}"
            LIBRARY DESTINATION "${CAQTDM_INSTALL_CONTROL_PLUGINDIR}"
            ARCHIVE DESTINATION "${CAQTDM_INSTALL_CONTROL_PLUGINDIR}")
    endif()
endfunction()
