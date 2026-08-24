# prc2ui: PSI .prc (PEP) to .ui converter (mirrors caQtDM_Viewer/parserPRC/parserPRC.pro).
# CLI driver around the static prc parser library.

add_executable(prc2ui
    prcParser.cpp)

target_include_directories(prc2ui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/prcParserSrc")

target_link_libraries(prc2ui PRIVATE
    prcParserStatic
    Qt6::Widgets)

if(WIN32)
    caqtdm_add_version_rc(prc2ui
        "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc" "PRC converter for Display Manager" "prc2ui.exe")
endif()

caqtdm_set_output_dir(prc2ui)
