# FindQwt
# -------
# Locates the Qwt library (6.x) and creates the imported target Qwt::Qwt.
#
# Cache variables (set by caQtDMConfig.cmake, all optional):
#   CAQTDM_QWT_HOME     Qwt installation prefix
#   CAQTDM_QWT_INCLUDE  Qwt include directory
#   CAQTDM_QWT_LIB      Qwt library directory
#   CAQTDM_QWT_LIBNAME  library base name (qwt, qwt-qt5, qwt-qt6, ...)
#
# Result variables:
#   Qwt_INCLUDE_DIR, Qwt_LIBRARY, Qwt_LIBRARY_DIR,
#   Qwt_VERSION_MAJOR, Qwt_VERSION_MINOR

if(NOT CAQTDM_QWT_LIBNAME)
    set(CAQTDM_QWT_LIBNAME qwt)
endif()

find_path(Qwt_INCLUDE_DIR NAMES qwt_global.h
    HINTS ${CAQTDM_QWT_INCLUDE} ${CAQTDM_QWT_HOME}/include ${CAQTDM_QWT_HOME}/lib/qwt.framework/Headers
    PATHS /usr/include/qwt /usr/include/qwt-qt6 /usr/include/qwt-qt5 /usr/local/include/qwt
    PATH_SUFFIXES qwt qwt-qt6 qwt-qt5)

find_library(Qwt_LIBRARY NAMES ${CAQTDM_QWT_LIBNAME}
    HINTS ${CAQTDM_QWT_LIB} ${CAQTDM_QWT_HOME}/lib
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
    PATH_SUFFIXES qwt qwt-qt6 qwt-qt5)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Qwt REQUIRED_VARS Qwt_LIBRARY Qwt_INCLUDE_DIR)

if(Qwt_FOUND AND NOT TARGET Qwt::Qwt)
    add_library(Qwt::Qwt UNKNOWN IMPORTED)
    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_LOCATION "${Qwt_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Qwt_INCLUDE_DIR}")
    if(APPLE AND Qwt_LIBRARY MATCHES "\\.framework$")
        set_target_properties(Qwt::Qwt PROPERTIES
            IMPORTED_LOCATION "${Qwt_LIBRARY}")
    endif()
endif()

get_filename_component(Qwt_LIBRARY_DIR "${Qwt_LIBRARY}" DIRECTORY)

set(Qwt_VERSION_MAJOR 0)
set(Qwt_VERSION_MINOR 0)
if(EXISTS "${Qwt_INCLUDE_DIR}/qwt_global.h")
    file(STRINGS "${Qwt_INCLUDE_DIR}/qwt_global.h" _qwt_version_line REGEX "#define[ \t]+QWT_VERSION[ \t]+0x[0-9a-fA-F]+")
    if(_qwt_version_line MATCHES "0x([0-9a-fA-F])([0-9a-fA-F])")
        math(EXPR Qwt_VERSION_MAJOR "0x${CMAKE_MATCH_1}" OUTPUT_FORMAT DECIMAL)
        math(EXPR Qwt_VERSION_MINOR "0x${CMAKE_MATCH_2}" OUTPUT_FORMAT DECIMAL)
    endif()
endif()
mark_as_advanced(Qwt_INCLUDE_DIR Qwt_LIBRARY)
