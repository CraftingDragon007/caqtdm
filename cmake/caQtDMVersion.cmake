# Parses the caQtDM version from qtdefs.pri so qmake and cmake share one
# source of truth. Included before project(); only file() commands used.

if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../caQtDM_Viewer/qtdefs.pri")
    message(FATAL_ERROR "qtdefs.pri not found - cannot determine the caQtDM version")
endif()

file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/../caQtDM_Viewer/qtdefs.pri" _caqtdm_version_lines
    REGEX "^CAQTDM_VERSION[ \t]*=[ \t]*V[0-9]+\\.[0-9]+\\.[0-9]+")
list(GET _caqtdm_version_lines 0 _caqtdm_version_line)
string(REGEX REPLACE "^CAQTDM_VERSION[ \t]*=[ \t]*V([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1"
    CAQTDM_VERSION_BASE "${_caqtdm_version_line}")

if(CAQTDM_VERSION_BASE STREQUAL "")
    message(WARNING "Could not parse CAQTDM_VERSION from qtdefs.pri, falling back to 4.9.0")
    set(CAQTDM_VERSION_BASE "4.9.0")
endif()
