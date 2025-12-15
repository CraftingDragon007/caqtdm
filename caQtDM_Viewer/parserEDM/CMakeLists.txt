if(NOT TARGET caqtdm::edlparser_static)
    message(FATAL_ERROR "edl2ui requires the edl parser static library.")
endif()

add_executable(edl2ui
    edlParser.cpp)

target_include_directories(edl2ui
    PRIVATE
        ${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc)

target_link_libraries(edl2ui
    PRIVATE
        caqtdm::edlparser_static)

caqtdm_target_build_info(edl2ui)
