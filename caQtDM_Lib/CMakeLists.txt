set(_caqtdm_lib_qt_components Core Gui Widgets Network UiTools)
if(QT_VERSION_MAJOR EQUAL 5)
    list(APPEND _caqtdm_lib_qt_components OpenGL)
else()
    list(APPEND _caqtdm_lib_qt_components OpenGLWidgets)
endif()
if(NOT ANDROID AND NOT IOS)
    list(APPEND _caqtdm_lib_qt_components PrintSupport)
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${_caqtdm_lib_qt_components})

file(GLOB LIB_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.c"
    "src/*.cpp"
    "src/*.c")

if(NOT CAQTDM_ENABLE_MOBILE)
    list(APPEND LIB_SOURCES src/myQProcess.cpp src/processWindow.cpp)
endif()

set(UI_FILES "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src/main.ui")

add_library(caqtdm_lib SHARED
    ${LIB_SOURCES}
    ${UI_FILES})

set_target_properties(caqtdm_lib PROPERTIES
    AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src"
    OUTPUT_NAME caQtDM_Lib)

if(WIN32)
    target_sources(caqtdm_lib PRIVATE src/caQtDM_Lib.rc)
endif()

add_library(caqtdm::lib ALIAS caqtdm_lib)

install(TARGETS caqtdm_lib
    RUNTIME DESTINATION "${CAQTDM_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CAQTDM_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CAQTDM_INSTALL_LIBDIR}")

caqtdm_target_version_info(caqtdm_lib
    PRODUCT "Library for Display Manager"
    FILENAME "caQtDM_Lib.dll")

target_compile_definitions(caqtdm_lib PRIVATE CAQTDM_LIB_LIBRARY)
if(QT_VERSION_MAJOR EQUAL 5)
    target_compile_definitions(caqtdm_lib PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)
elseif(QT_VERSION_MAJOR EQUAL 6)
    target_compile_definitions(caqtdm_lib PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x050000)
endif()

# Windows-specific compiler definitions
if(MSVC)
    target_compile_definitions(caqtdm_lib PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set(_caqtdm_lib_includes
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/caQtDM_Plugins
    ${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src
    ${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc
    ${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc
    ${CAQTDM_QWT_INCLUDE_DIR}
    ${CAQTDM_EPICS_INCLUDE_DIR})

if(WIN32)
    list(APPEND _caqtdm_lib_includes
        ${CAQTDM_EPICS_INCLUDE_DIR}/os/win32)
    if(MSVC)
        list(APPEND _caqtdm_lib_includes
            ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/msvc)
    elseif(MINGW)
        list(APPEND _caqtdm_lib_includes
            ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/gcc)
    endif()
elseif(UNIX AND NOT APPLE)
    list(APPEND _caqtdm_lib_includes
        ${CAQTDM_EPICS_INCLUDE_DIR}/os/Linux
        ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/gcc)
elseif(APPLE)
    list(APPEND _caqtdm_lib_includes
        ${CAQTDM_EPICS_INCLUDE_DIR}/os/Darwin
        ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/clang
        ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/gcc)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND _caqtdm_lib_includes
        ${CAQTDM_EPICS_INCLUDE_DIR}/os/freebsd
        ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/clang)
endif()

if(CAQTDM_ANDROID_FUNCTIONS_INCLUDE)
    list(APPEND _caqtdm_lib_includes ${CAQTDM_ANDROID_FUNCTIONS_INCLUDE})
endif()

list(REMOVE_DUPLICATES _caqtdm_lib_includes)

target_include_directories(caqtdm_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE
        ${_caqtdm_lib_includes})

set(_caqtdm_lib_links
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::UiTools
    caqtdm::qwt
    caqtdm::qtcontrols
    ${CAQTDM_EPICS_CORE_LIBRARIES})

if(QT_VERSION_MAJOR EQUAL 5)
    list(APPEND _caqtdm_lib_links Qt5::OpenGL)
else()
    list(APPEND _caqtdm_lib_links Qt6::OpenGLWidgets)
endif()

if(NOT ANDROID AND NOT IOS)
    list(APPEND _caqtdm_lib_links Qt${QT_VERSION_MAJOR}::PrintSupport)
endif()

if(CAQTDM_QT_CORE5COMPAT_TARGET)
    list(APPEND _caqtdm_lib_links ${CAQTDM_QT_CORE5COMPAT_TARGET})
endif()

target_link_libraries(caqtdm_lib PRIVATE ${_caqtdm_lib_links})

if(CAQTDM_ENABLE_PYTHONCALC)
    if(NOT CAQTDM_PYTHON_LIBRARY)
        set(_python_candidates "python${CAQTDM_PYTHON_VERSION}" "python${CAQTDM_PYTHON_VERSION}m")
        find_library(CAQTDM_PYTHON_LIBRARY
            NAMES ${_python_candidates}
            HINTS ${CAQTDM_PYTHON_LIBRARY_DIR})
    endif()
    if(NOT CAQTDM_PYTHON_LIBRARY)
        message(FATAL_ERROR "Could not find Python library for version ${CAQTDM_PYTHON_VERSION}. Set CAQTDM_PYTHON_LIBRARY or disable CAQTDM_ENABLE_PYTHONCALC.")
    endif()
    set(CAQTDM_PYTHON_LIBRARY ${CAQTDM_PYTHON_LIBRARY} CACHE FILEPATH "Python library used for optional calculations" FORCE)
    target_include_directories(caqtdm_lib PRIVATE ${CAQTDM_PYTHON_INCLUDE_DIR})
    target_link_libraries(caqtdm_lib PRIVATE ${CAQTDM_PYTHON_LIBRARY})
    target_compile_definitions(caqtdm_lib PRIVATE PYTHON)
endif()

if(CAQTDM_ENABLE_RPATH)
    set(_caqtdm_rpaths)
    foreach(dir IN LISTS CAQTDM_EPICS_LIBRARY_DIR CAQTDM_QWT_LIBRARY_DIR CAQTDM_PYTHON_LIBRARY_DIR CAQTDM_ZMQ_LIBRARY_DIR)
        if(dir AND IS_DIRECTORY "${dir}")
            list(APPEND _caqtdm_rpaths "${dir}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _caqtdm_rpaths)
    if(_caqtdm_rpaths)
        set_target_properties(caqtdm_lib PROPERTIES BUILD_RPATH "${_caqtdm_rpaths}")
    endif()
endif()
