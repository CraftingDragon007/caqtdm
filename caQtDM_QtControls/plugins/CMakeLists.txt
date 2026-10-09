# The four widget plugin groups, loaded by Designer on desktop and linked
# statically into the application on mobile.

set(_plugin_groups controllers graphics monitors utilities)

foreach(_group ${_plugin_groups})
    set(_target qtcontrols_${_group}_plugin)
    if(_group STREQUAL "controllers")
        set(_class_name CustomWidgetCollectionInterface_Controllers)
    elseif(_group STREQUAL "graphics")
        set(_class_name CustomWidgetCollectionInterface_Graphics)
    elseif(_group STREQUAL "monitors")
        set(_class_name CustomWidgetCollectionInterface_Monitors)
    else()
        set(_class_name CustomWidgetCollectionInterface_Utilities)
    endif()
    if(CAQTDM_MOBILE)
        qt_add_plugin(${_target} STATIC CLASS_NAME "${_class_name}"
            "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrols_${_group}_plugin.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrols_${_group}_plugin.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/designerPluginTexts.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/plugin_xml_helper.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrolsplugin.qrc")
    else()
        add_library(${_target} MODULE
            "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrols_${_group}_plugin.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrols_${_group}_plugin.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/designerPluginTexts.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/plugin_xml_helper.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrolsplugin.qrc")
    endif()
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src")
    target_link_libraries(${_target}
        PRIVATE
            qtcontrols
            Qwt::Qwt
            ${CAQTDM_QT_PACKAGE}::UiTools
            ${CAQTDM_QT_PACKAGE}::Widgets
            ${CAQTDM_QT_PACKAGE}::OpenGL)
    if(CAQTDM_MOBILE)
        target_link_libraries(${_target} PRIVATE ${CAQTDM_QT_PACKAGE}::UiPlugin)
    else()
        target_link_libraries(${_target} PRIVATE ${CAQTDM_QT_PACKAGE}::Designer)
    endif()
    target_compile_definitions(${_target} PRIVATE CAHMI)
    if(MSVC)
        target_compile_definitions(${_target} PRIVATE CAQTDM_PLUGIN_LIBRARY _CRT_SECURE_NO_WARNINGS)
    endif()
    caqtdm_set_output_dir(${_target} SUBDIR designer)
    caqtdm_apply_rpath(${_target})
    if(NOT CAQTDM_MOBILE)
        install(TARGETS ${_target}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/designer
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/designer)
    endif()
endforeach()
