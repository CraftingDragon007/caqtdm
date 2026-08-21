# The four Qt Designer plugin groups (mirrors plugins/plugins.pri and the
# qtcontrols_{controllers,graphics,monitors,utilities}.pro files).

set(_plugin_groups controllers graphics monitors utilities)

foreach(_group ${_plugin_groups})
    set(_target qtcontrols_${_group}_plugin)
    add_library(${_target} MODULE
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
            Qt6::Designer
            Qt6::UiTools
            Qt6::Widgets
            Qt6::OpenGL)
    target_compile_definitions(${_target} PRIVATE CAHMI)
    if(MSVC)
        target_compile_definitions(${_target} PRIVATE CAQTDM_PLUGIN_LIBRARY _CRT_SECURE_NO_WARNINGS)
    endif()
    caqtdm_set_output_dir(${_target} SUBDIR designer)
    caqtdm_apply_rpath(${_target})
    install(TARGETS ${_target} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/designer)
endforeach()
