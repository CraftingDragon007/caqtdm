cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(caQtDMVersion)

project(caQtDM VERSION ${CAQTDM_VERSION_BASE} LANGUAGES C CXX)
set(PROJECT_VERSION_STR "${CAQTDM_VERSION_STR}")

include(caQtDMConfig)
include(caQtDMHelpers)

add_subdirectory(caQtDM_Parsers)
add_subdirectory(caQtDM_QtControls)
add_subdirectory(caQtDM_Lib)
add_subdirectory(caQtDM_Viewer)
add_subdirectory(caQtDM_Plugins)

if(CAQTDM_WITH_TESTS AND NOT CAQTDM_MOBILE)
    enable_testing()
    add_subdirectory(caQtDM_UnitTests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES
    DESCRIPTION "--- caQtDM ---")
