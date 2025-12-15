add_executable(adl2ui
    adlParser.cpp)

target_include_directories(adl2ui
    PRIVATE
        ${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc)

target_link_libraries(adl2ui
    PRIVATE
        caqtdm::adlparser_static)

caqtdm_target_build_info(adl2ui)
caqtdm_target_version_info(adl2ui
    PRODUCT "ADL converter for Display Manager"
    FILENAME "adl2ui.exe")

if(WIN32)
    target_sources(adl2ui PRIVATE ${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc)
endif()
