# SPDX-License-Identifier: GPL-3.0-or-later
# Utilities to construct Qt Designer plugins for caQtDM controls.

include_guard(GLOBAL)

function(caqtdm_add_qtcontrols_plugin target)
    set(options RESOURCE_ON_MOBILE)
    set(oneValueArgs SOURCE_BASE)
    cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT PLUGIN_SOURCE_BASE)
        message(FATAL_ERROR "caqtdm_add_qtcontrols_plugin requires SOURCE_BASE")
    endif()

    set(_plugin_type MODULE)
    if(IOS OR ANDROID)
        set(_plugin_type STATIC)
    endif()

    set(_plugin_root "${CMAKE_CURRENT_SOURCE_DIR}/..")
    set(_plugin_sources
        "${_plugin_root}/${PLUGIN_SOURCE_BASE}_plugin.cpp"
        "${_plugin_root}/${PLUGIN_SOURCE_BASE}_plugin.h"
        "${_plugin_root}/designerPluginTexts.h")

    add_library(${target} ${_plugin_type} ${_plugin_sources})

    set_target_properties(${target} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CAQTDM_BUILD_BINDIR}/designer")

    if(PLUGIN_RESOURCE_ON_MOBILE OR NOT CAQTDM_ENABLE_MOBILE)
        target_sources(${target} PRIVATE "${_plugin_root}/qtcontrolsplugin.qrc")
    endif()

    set(_qt_components Widgets UiTools)
    if(QT_VERSION_MAJOR EQUAL 5)
        list(APPEND _qt_components OpenGL)
    else()
        list(APPEND _qt_components OpenGLWidgets)
    endif()
    if(NOT CAQTDM_ENABLE_MOBILE)
        list(APPEND _qt_components Designer)
    endif()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${_qt_components})

    target_include_directories(${target}
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${_plugin_root}"
            "${_plugin_root}/.."
            "${_plugin_root}/../src"
            "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src"
            "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc"
            "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc"
            "${CAQTDM_QWT_INCLUDE_DIR}")

    if(ANDROID AND CAQTDM_ANDROID_FUNCTIONS_INCLUDE)
        target_include_directories(${target} PRIVATE ${CAQTDM_ANDROID_FUNCTIONS_INCLUDE})
    endif()

    target_link_libraries(${target}
        PRIVATE
            caqtdm::qtcontrols
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::UiTools)

    if(QT_VERSION_MAJOR EQUAL 5)
        target_link_libraries(${target} PRIVATE Qt5::OpenGL)
    else()
        target_link_libraries(${target} PRIVATE Qt6::OpenGLWidgets)
    endif()

    if(NOT CAQTDM_ENABLE_MOBILE AND TARGET Qt${QT_VERSION_MAJOR}::Designer)
        target_link_libraries(${target} PRIVATE Qt${QT_VERSION_MAJOR}::Designer)
    endif()

    # Install designer plugins only when built as MODULE
    if(NOT ANDROID AND NOT IOS)
        install(TARGETS ${target}
            RUNTIME DESTINATION "${CAQTDM_INSTALL_DESIGNER_PLUGINDIR}"
            LIBRARY DESTINATION "${CAQTDM_INSTALL_DESIGNER_PLUGINDIR}"
            ARCHIVE DESTINATION "${CAQTDM_INSTALL_DESIGNER_PLUGINDIR}")
    endif()
endfunction()
