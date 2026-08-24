# Parser libraries: adl (MEDM), edl (EDM, Unix only) and prc (PEP panels).
# Each parser is built as a static library (linked into libqtcontrols /
# the converter tools) and as a shared library (runtime conversion in the
# viewer). Mirrors caQtDM_Parsers.pro.

set(_parsers_src "${CMAKE_CURRENT_SOURCE_DIR}")

# Build stamps for all parsers and converter tools (caQtDM_xdl2ui scope)
string(TIMESTAMP CAQTDM_BUILDTIME "%H:%M")
string(TIMESTAMP CAQTDM_BUILDDATE "%d-%m-%Y")
string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" CAQTDM_BUILDARCH)
set(CAQTDM_PARSER_DEFINES
    BUILDVERSION=\"V${PROJECT_VERSION}\"
    BUILDARCH=\"${CAQTDM_BUILDARCH}\")

# --------------------------------------------------------------------------------------------------
# ADL parser (MEDM .adl)
# --------------------------------------------------------------------------------------------------
set(ADL_SOURCES
    ${_parsers_src}/adlParserSrc/XmlWriter.cpp
    ${_parsers_src}/adlParserSrc/parser.c
    ${_parsers_src}/adlParserSrc/adlParserMain.cpp
    ${_parsers_src}/adlParserSrc/QtProperties.c
    ${_parsers_src}/adlParserSrc/dmsearchfile.cpp)
set(ADL_HEADERS
    ${_parsers_src}/adlParserSrc/XmlWriter.h
    ${_parsers_src}/adlParserSrc/adlParserMain.h
    ${_parsers_src}/adlParserSrc/QtProperties.h
    ${_parsers_src}/adlParserSrc/dmsearchfile.h)

add_library(adlParserStatic STATIC ${ADL_SOURCES} ${ADL_HEADERS})
target_include_directories(adlParserStatic PUBLIC "${_parsers_src}/adlParserSrc")
target_link_libraries(adlParserStatic PUBLIC Qt6::Widgets)
set_target_properties(adlParserStatic PROPERTIES OUTPUT_NAME adlParser)

add_library(adlParser SHARED ${ADL_SOURCES} ${ADL_HEADERS})
target_include_directories(adlParser PUBLIC "${_parsers_src}/adlParserSrc")
target_link_libraries(adlParser PUBLIC Qt6::Widgets)
set_target_properties(adlParser PROPERTIES OUTPUT_NAME adlParser)

if(WIN32)
    caqtdm_add_version_rc(adlParser
        "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc" "ADL converter library for Display Manager" "adlParser.dll")
endif()

# --------------------------------------------------------------------------------------------------
# EDL parser (EDM .edl) - Unix only, like qmake (edlParserStaticLib/edlParserSharedLib)
# --------------------------------------------------------------------------------------------------
if(UNIX)
    set(EDL_SOURCES
        ${_parsers_src}/edlParserSrc/XmlWriter.cpp
        ${_parsers_src}/edlParserSrc/edlParserMain.cpp
        ${_parsers_src}/edlParserSrc/dmsearchfile.cpp
        ${_parsers_src}/edlParserSrc/tag_pkg.cc
        ${_parsers_src}/edlParserSrc/utility.cc
        ${_parsers_src}/edlParserSrc/expString.cc
        ${_parsers_src}/edlParserSrc/parserClass.cc)
    set(EDL_HEADERS
        ${_parsers_src}/edlParserSrc/XmlWriter.h
        ${_parsers_src}/edlParserSrc/edlParserMain.h
        ${_parsers_src}/edlParserSrc/dmsearchfile.h
        ${_parsers_src}/edlParserSrc/tag_pkg.h
        ${_parsers_src}/edlParserSrc/utility.h
        ${_parsers_src}/edlParserSrc/expString.h
        ${_parsers_src}/edlParserSrc/parserClass.h)

    foreach(_kind Static Shared)
        if(_kind STREQUAL "Static")
            add_library(edlParserStatic STATIC ${EDL_SOURCES} ${EDL_HEADERS})
            set(_tgt edlParserStatic)
        else()
            add_library(edlParser SHARED ${EDL_SOURCES} ${EDL_HEADERS})
            set(_tgt edlParser)
        endif()
        target_include_directories(${_tgt} PUBLIC "${_parsers_src}/edlParserSrc")
        target_link_libraries(${_tgt} PUBLIC Qt6::Widgets)
        target_compile_options(${_tgt} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-write-strings>)
        set_target_properties(${_tgt} PROPERTIES OUTPUT_NAME edlParser)
    endforeach()
endif()

# --------------------------------------------------------------------------------------------------
# PRC parser (PSI .prc / PEP panels)
# --------------------------------------------------------------------------------------------------
set(PRC_SOURCES
    ${_parsers_src}/prcParserSrc/parseprcfile.cpp
    ${_parsers_src}/prcParserSrc/prctokenizer.cpp
    ${_parsers_src}/prcParserSrc/prcuiwriter.cpp)
set(PRC_HEADERS
    ${_parsers_src}/prcParserSrc/parseprcfile.h
    ${_parsers_src}/prcParserSrc/prctokenizer.h
    ${_parsers_src}/prcParserSrc/prcuiwriter.h
    ${_parsers_src}/prcParserSrc/prcparserdefs.h)

add_library(prcParserStatic STATIC ${PRC_SOURCES} ${PRC_HEADERS})
target_include_directories(prcParserStatic PUBLIC "${_parsers_src}/prcParserSrc")
target_link_libraries(prcParserStatic PUBLIC Qt6::Widgets Qt6::UiTools)
set_target_properties(prcParserStatic PROPERTIES OUTPUT_NAME prcParser)

add_library(prcParser SHARED ${PRC_SOURCES} ${PRC_HEADERS})
target_include_directories(prcParser PUBLIC "${_parsers_src}/prcParserSrc")
target_link_libraries(prcParser PUBLIC Qt6::Widgets Qt6::UiTools)
target_compile_definitions(prcParser PRIVATE PRCPARSER_MAKEDLL)
set_target_properties(prcParser PROPERTIES OUTPUT_NAME prcParser)

if(WIN32)
    caqtdm_add_version_rc(prcParser
        "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc" "PRC converter library for Display Manager" "prcParser.dll")
endif()

# On Windows the import lib of a shared target and a static library with
# the same name collide in one directory - give the statics a suffix there.
if(WIN32)
    set_target_properties(adlParserStatic PROPERTIES OUTPUT_NAME adlParser_static)
    set_target_properties(prcParserStatic PROPERTIES OUTPUT_NAME prcParser_static)
endif()

foreach(_tgt adlParserStatic adlParser prcParserStatic prcParser)
    target_compile_definitions(${_tgt} PRIVATE ${CAQTDM_PARSER_DEFINES})
endforeach()
if(UNIX)
    foreach(_tgt edlParserStatic edlParser)
        target_compile_definitions(${_tgt} PRIVATE ${CAQTDM_PARSER_DEFINES})
    endforeach()
endif()

# --------------------------------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------------------------------
install(TARGETS adlParserStatic adlParser prcParserStatic prcParser
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX)
    install(TARGETS edlParserStatic edlParser
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
