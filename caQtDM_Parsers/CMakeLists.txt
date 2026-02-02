find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(_adl_parser_sources
    adlParserSrc/XmlWriter.cpp
    adlParserSrc/parser.c
    adlParserSrc/adlParserMain.cpp
    adlParserSrc/QtProperties.c
    adlParserSrc/dmsearchfile.cpp)

function(_caqtdm_configure_parser_target target include_dir)
    target_include_directories(${target}
        PUBLIC
            ${include_dir})
    target_link_libraries(${target}
        PUBLIC
            Qt${QT_VERSION_MAJOR}::Widgets)
    target_compile_definitions(${target} PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)
    caqtdm_target_build_info(${target})
endfunction()

add_library(adlParser_static STATIC ${_adl_parser_sources})
# On Windows, differentiate static and shared library names to avoid .lib conflicts
if(WIN32)
    set_target_properties(adlParser_static PROPERTIES OUTPUT_NAME adlParser_static)
else()
    set_target_properties(adlParser_static PROPERTIES OUTPUT_NAME adlParser)
endif()
_caqtdm_configure_parser_target(adlParser_static "${CMAKE_CURRENT_SOURCE_DIR}/adlParserSrc")
add_library(caqtdm::adlparser_static ALIAS adlParser_static)

install(TARGETS adlParser_static
    ARCHIVE DESTINATION "${CAQTDM_INSTALL_LIBDIR}")

if(NOT CAQTDM_ENABLE_MOBILE)
    add_library(adlParser_shared SHARED ${_adl_parser_sources})
    set_target_properties(adlParser_shared PROPERTIES OUTPUT_NAME adlParser)
    _caqtdm_configure_parser_target(adlParser_shared "${CMAKE_CURRENT_SOURCE_DIR}/adlParserSrc")
    caqtdm_target_version_info(adlParser_shared
        PRODUCT "ADL converter library for Display Manager"
        FILENAME "adlParser.dll")
    if(WIN32)
        target_sources(adlParser_shared PRIVATE ${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc)
    endif()
    add_library(caqtdm::adlparser_shared ALIAS adlParser_shared)

    install(TARGETS adlParser_shared
        RUNTIME DESTINATION "${CAQTDM_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CAQTDM_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CAQTDM_INSTALL_LIBDIR}")
endif()

if(TARGET adlParser_shared)
    add_library(caqtdm::adlparser ALIAS adlParser_shared)
else()
    add_library(caqtdm::adlparser ALIAS adlParser_static)
endif()

if(UNIX)
    set(_edl_parser_sources
        edlParserSrc/XmlWriter.cpp
        edlParserSrc/edlParserMain.cpp
        edlParserSrc/dmsearchfile.cpp
        edlParserSrc/tag_pkg.cc
        edlParserSrc/utility.cc
        edlParserSrc/expString.cc
        edlParserSrc/parserClass.cc)

    add_library(edlParser_static STATIC ${_edl_parser_sources})
    set_target_properties(edlParser_static PROPERTIES OUTPUT_NAME edlParser)
    _caqtdm_configure_parser_target(edlParser_static "${CMAKE_CURRENT_SOURCE_DIR}/edlParserSrc")
    target_compile_options(edlParser_static PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-write-strings>)
    add_library(caqtdm::edlparser_static ALIAS edlParser_static)

    install(TARGETS edlParser_static
        ARCHIVE DESTINATION "${CAQTDM_INSTALL_LIBDIR}")

    if(NOT CAQTDM_ENABLE_MOBILE)
        add_library(edlParser_shared SHARED ${_edl_parser_sources})
        set_target_properties(edlParser_shared PROPERTIES OUTPUT_NAME edlParser)
        _caqtdm_configure_parser_target(edlParser_shared "${CMAKE_CURRENT_SOURCE_DIR}/edlParserSrc")
        target_compile_options(edlParser_shared PRIVATE
            $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-write-strings>)
        caqtdm_target_version_info(edlParser_shared
            PRODUCT "EDL converter library for Display Manager"
            FILENAME "edlParser.dll")
        if(WIN32)
            target_sources(edlParser_shared PRIVATE ${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/caQtDM.rc)
        endif()
        add_library(caqtdm::edlparser_shared ALIAS edlParser_shared)

        install(TARGETS edlParser_shared
            RUNTIME DESTINATION "${CAQTDM_INSTALL_BINDIR}"
            LIBRARY DESTINATION "${CAQTDM_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CAQTDM_INSTALL_LIBDIR}")
    endif()

    if(TARGET edlParser_shared)
        add_library(caqtdm::edlparser ALIAS edlParser_shared)
    else()
        add_library(caqtdm::edlparser ALIAS edlParser_static)
    endif()
endif()
