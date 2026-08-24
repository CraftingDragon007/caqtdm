# edl2ui: EDM .edl to .ui converter (mirrors caQtDM_Viewer/parserEDM/parserEDM.pro).
# Links the static edl parser library.

add_executable(edl2ui
    edlParser.h
    edlParser.cpp)

target_include_directories(edl2ui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc")

target_link_libraries(edl2ui PRIVATE
    edlParserStatic
    Qt6::Widgets)

caqtdm_set_output_dir(edl2ui)
