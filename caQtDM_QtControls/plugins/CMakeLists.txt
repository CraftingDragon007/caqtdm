# The four widget plugin groups, loaded by Designer on desktop and linked
# statically into the application on mobile.

set(_plugin_groups controllers graphics monitors utilities)

foreach(_group ${_plugin_groups})
    set(_target qtcontrols_${_group}_plugin)
    if(CAQTDM_MOBILE)
        set(_plugin_type STATIC)
    else()
        set(_plugin_type MODULE)
    endif()
    add_library(${_target} ${_plugin_type}
        "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrols_${_group}_plugin.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrols_${_group}_plugin.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/designerPluginTexts.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/plugin_xml_helper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/qtcontrolsplugin.qrc")
    target_include_directories(${_target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src")
    target_link_libraries(${_target}
        PRIVATE
            qtcontrols
            Qwt::Qwt
            Qt6::UiTools
            Qt6::Widgets
            Qt6::OpenGL)
    if(CAQTDM_MOBILE)
        target_link_libraries(${_target} PRIVATE Qt6::UiPlugin)
    else()
        target_link_libraries(${_target} PRIVATE Qt6::Designer)
    endif()
    target_compile_definitions(${_target} PRIVATE CAHMI)
    if(MSVC)
        target_compile_definitions(${_target} PRIVATE CAQTDM_PLUGIN_LIBRARY _CRT_SECURE_NO_WARNINGS)
    endif()
    caqtdm_set_output_dir(${_target} SUBDIR designer)
    caqtdm_apply_rpath(${_target})
    install(TARGETS ${_target}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/designer
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/designer)
endforeach()
