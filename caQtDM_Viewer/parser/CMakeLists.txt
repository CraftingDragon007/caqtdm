# adl2ui: MEDM .adl to .ui converter (mirrors caQtDM_Viewer/parser/parser.pro).
# Links the static adl AND edl parser libraries (caQtDM_xdl2ui_Lib).

add_executable(adl2ui
    adlParser.h
    adlParser.cpp)

target_include_directories(adl2ui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc")

target_link_libraries(adl2ui PRIVATE
    adlParserStatic
    edlParserStatic)

if(MSVC)
    target_sources(adl2ui PRIVATE "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc")
endif()

caqtdm_set_output_dir(adl2ui)
