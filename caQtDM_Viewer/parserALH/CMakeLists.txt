# Optional diagnostic tool; intentionally not installed or packaged.
add_executable(alh2ui alh2ui.cpp)
target_include_directories(alh2ui PRIVATE
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/alhParserSrc"
    "${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src")
target_link_libraries(alh2ui PRIVATE alhParserStatic ${CAQTDM_QT_PACKAGE}::Widgets ${CAQTDM_QT_PACKAGE}::UiTools)
caqtdm_set_output_dir(alh2ui)
