# adl2ui: MEDM .adl to .ui converter (mirrors caQtDM_Viewer/parser/parser.pro).
# Links the static adl AND edl parser libraries (caQtDM_xdl2ui_Lib).

add_executable(adl2ui
    adlParser.h
    adlParser.cpp)

target_include_directories(adl2ui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc")

if(WIN32)
    target_link_libraries(adl2ui PRIVATE adlParserStatic)
else()
    target_link_libraries(adl2ui PRIVATE
        adlParserStatic
        edlParserStatic)
endif()
target_link_libraries(adl2ui PRIVATE Qt6::Widgets)

if(WIN32)
    caqtdm_add_version_rc(adl2ui
        "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc" "ADL converter for Display Manager" "adl2ui.exe")
endif()

caqtdm_set_output_dir(adl2ui)
