set(_qtcontrols_qt_components Gui Widgets Network Concurrent UiTools)
if(QT_VERSION_MAJOR EQUAL 5)
    list(APPEND _qtcontrols_qt_components OpenGL)
else()
    list(APPEND _qtcontrols_qt_components OpenGLWidgets)
endif()
if(NOT ANDROID AND NOT IOS)
    list(APPEND _qtcontrols_qt_components PrintSupport)
endif()
if(NOT CAQTDM_ENABLE_MOBILE)
    list(APPEND _qtcontrols_qt_components Designer)
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${_qtcontrols_qt_components})

file(GLOB QTCONTROLS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
set(_qtcontrols_conditional_sources
    src/parseotherfile.cpp
    src/qwt_thermo_marker.cpp
    src/qwt_thermo_marker_61.cpp
    src/xdr_hack.c
    src/pvtaskmenu.cpp
    src/pvdialog.cpp
    src/cadoubletabwidgetextensionfactory.cpp
    src/cadoubletabwidgetextension.cpp
    src/capolylinetaskmenu.cpp
    src/capolylinedialog.cpp)
foreach(_src IN LISTS _qtcontrols_conditional_sources)
    list(REMOVE_ITEM QTCONTROLS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
endforeach()

add_library(qtcontrols SHARED
    ${QTCONTROLS_SOURCES}
    qtcontrols.qrc)

# Header-only QObject types need explicit moc processing
target_sources(qtcontrols PRIVATE src/networkmodel.h)

caqtdm_target_version_info(qtcontrols
    PRODUCT "Widgets for Display Manager"
    FILENAME "qtcontrols.dll")

if(WIN32)
    target_sources(qtcontrols PRIVATE src/qtcontrols.rc)
endif()

if(CAQTDM_ENABLE_ADL_EDL)
    target_sources(qtcontrols PRIVATE src/parseotherfile.cpp)
endif()

if(CAQTDM_ENABLE_XDR_HACK)
    target_sources(qtcontrols PRIVATE src/xdr_hack.c)
endif()

if(CAQTDM_QWT_VERSION_MINOR_NUM GREATER 0)
    target_sources(qtcontrols PRIVATE src/qwt_thermo_marker_61.cpp)
else()
    target_sources(qtcontrols PRIVATE src/qwt_thermo_marker.cpp)
endif()

if(NOT CAQTDM_ENABLE_MOBILE)
    target_sources(qtcontrols PRIVATE
        src/pvtaskmenu.cpp
        src/pvdialog.cpp
        src/cadoubletabwidgetextensionfactory.cpp
        src/cadoubletabwidgetextension.cpp
        src/capolylinetaskmenu.cpp
        src/capolylinedialog.cpp)
endif()

set_target_properties(qtcontrols PROPERTIES OUTPUT_NAME qtcontrols)

if(QT_VERSION_MAJOR EQUAL 5)
    target_compile_definitions(qtcontrols PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x000000)
elseif(QT_VERSION_MAJOR EQUAL 6)
    target_compile_definitions(qtcontrols PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x050000)
endif()

# Windows-specific compiler definitions
if(MSVC)
    target_compile_definitions(qtcontrols PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        QTCON_MAKEDLL)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_include_directories(qtcontrols PRIVATE /usr/local/include)
    target_link_libraries(qtcontrols PRIVATE z)
endif()

if(ANDROID AND CAQTDM_ANDROID_FUNCTIONS_INCLUDE)
    target_include_directories(qtcontrols PRIVATE ${CAQTDM_ANDROID_FUNCTIONS_INCLUDE})
endif()

target_include_directories(qtcontrols
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/caQtDM_Lib/src
        ${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc
        ${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc)

if(NOT TARGET caqtdm::qtcontrols)
    add_library(caqtdm::qtcontrols ALIAS qtcontrols)
endif()

install(TARGETS qtcontrols
    RUNTIME DESTINATION "${CAQTDM_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CAQTDM_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CAQTDM_INSTALL_LIBDIR}")

target_link_libraries(qtcontrols
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        caqtdm::qwt
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::Concurrent
        Qt${QT_VERSION_MAJOR}::UiTools)

if(QT_VERSION_MAJOR EQUAL 5)
    target_link_libraries(qtcontrols PRIVATE Qt5::OpenGL)
else()
    target_link_libraries(qtcontrols PRIVATE Qt6::OpenGLWidgets)
endif()

if(NOT ANDROID AND NOT IOS)
    target_link_libraries(qtcontrols PRIVATE Qt${QT_VERSION_MAJOR}::PrintSupport)
endif()

if(NOT CAQTDM_ENABLE_MOBILE AND TARGET Qt${QT_VERSION_MAJOR}::Designer)
    target_link_libraries(qtcontrols PRIVATE Qt${QT_VERSION_MAJOR}::Designer)
endif()

if(CAQTDM_ENABLE_ADL_EDL)
    # On Windows, embed parser functionality using static libraries to avoid DLL dependencies
    # On other platforms, prefer static libraries for embedding as well
    if(TARGET adlParser_static)
        target_link_libraries(qtcontrols PRIVATE adlParser_static)
        add_dependencies(qtcontrols adlParser_static)
    endif()
    if(TARGET edlParser_static)
        target_link_libraries(qtcontrols PRIVATE edlParser_static)
        add_dependencies(qtcontrols edlParser_static)
    endif()
endif()
