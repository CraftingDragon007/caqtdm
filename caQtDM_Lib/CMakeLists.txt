# caQtDM_Lib: the central dispatcher library (mirrors caQtDM_Lib.pro).

set(_src "${CMAKE_CURRENT_SOURCE_DIR}/src")

set(LIB_SOURCES
    ${_src}/caqtdm_lib.cpp
    ${_src}/mutexKnobData.cpp
    ${_src}/MessageWindow.cpp
    ${_src}/causerid.cpp
    ${_src}/vaPrintf.c
    ${_src}/myMessageBox.cpp
    ${_src}/limitsStripplotDialog.cpp
    ${_src}/limitsCartesianplotDialog.cpp
    ${_src}/limitsDialog.cpp
    ${_src}/sliderDialog.cpp
    ${_src}/splashscreen.cpp
    ${_src}/loadPlugins.cpp
    ${_src}/hmisharedconfiglistmanager.cpp
    ${_src}/hmisharedeventbus.cpp)

set(LIB_HEADERS
    ${_src}/caqtdm_lib.h
    ${_src}/caQtDM_Lib_global.h
    ${_src}/mutexKnobDataWrapper.h
    ${_src}/mutexKnobData.h
    ${_src}/knobDefines.h
    ${_src}/knobData.h
    ${_src}/dbrString.h
    ${_src}/alarmstrings.h
    ${_src}/MessageWindow.h
    ${_src}/messageWindowWrapper.h
    ${_src}/vaPrintf.h
    ${_src}/myMessageBox.h
    ${_src}/limitsStripplotDialog.h
    ${_src}/limitsDialog.h
    ${_src}/limitsCartesianplotDialog.h
    ${_src}/sliderDialog.h
    ${_src}/splashscreen.h
    ${_src}/epicsExternals.h
    ${_src}/inlines.h
    ${_src}/loadPlugins.h
    ${_src}/caqtdm_lib_interface.h
    ${_src}/causerid.h
    ${_src}/hmi_common_event_defs.h
    ${_src}/hmisharedconfiglistmanager.h
    ${_src}/hmisharedeventbus.h)

if(NOT CAQTDM_MOBILE)
    list(APPEND LIB_SOURCES ${_src}/myQProcess.cpp ${_src}/processWindow.cpp)
    list(APPEND LIB_HEADERS ${_src}/myQProcess.h ${_src}/processWindow.h)
else()
    list(APPEND LIB_SOURCES ${_src}/fingerswipegesture.cpp)
    list(APPEND LIB_HEADERS ${_src}/fingerswipegesture.h)
endif()

if(CAQTDM_HAVE_WEBSOCKETS)
    list(APPEND LIB_SOURCES
        ${_src}/websocketserver.cpp
        ${_src}/vncwebchildprocess.cpp
        ${_src}/webportpool.cpp
        ${_src}/weblaunchermanager.cpp)
    list(APPEND LIB_HEADERS
        ${_src}/websocketserver.h
        ${_src}/vncwebchildprocess.h
        ${_src}/webportpool.h
        ${_src}/weblaunchermanager.h)
endif()

if(CAQTDM_MOBILE)
    add_library(caQtDM_Lib STATIC ${LIB_SOURCES} ${LIB_HEADERS})
else()
    add_library(caQtDM_Lib SHARED ${LIB_SOURCES} ${LIB_HEADERS})
endif()

add_library(caqtdm::lib ALIAS caQtDM_Lib)

set_target_properties(caQtDM_Lib PROPERTIES AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src")

target_include_directories(caQtDM_Lib
    PUBLIC
        "${_src}"
        "${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src"
        "${PROJECT_SOURCE_DIR}/caQtDM_Plugins"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc"
        "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc")

target_link_libraries(caQtDM_Lib
    PUBLIC
        qtcontrols
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::UiTools
        Qt6::OpenGL
        Qt6::Xml
    PRIVATE
        Epics::Com)

if(NOT CAQTDM_MOBILE)
    target_link_libraries(caQtDM_Lib PUBLIC Qt6::PrintSupport)
endif()

if(CAQTDM_HAVE_WEBSOCKETS)
    target_link_libraries(caQtDM_Lib PUBLIC Qt6::WebSockets)
    target_compile_definitions(caQtDM_Lib PUBLIC WEB)
endif()

if(CAQTDM_HAVE_PYTHON)
    target_link_libraries(caQtDM_Lib PRIVATE caqtdm::python)
    target_compile_definitions(caQtDM_Lib PRIVATE PYTHON)
endif()

target_compile_definitions(caQtDM_Lib PRIVATE QT_DISABLE_DEPRECATED_BEFORE=0x050000)

if(MSVC)
    target_compile_definitions(caQtDM_Lib PRIVATE CAQTDM_LIB_LIBRARY _CRT_SECURE_NO_WARNINGS)
    target_sources(caQtDM_Lib PRIVATE "${_src}/caQtDM_Lib.rc")
elseif(WIN32)
    target_link_libraries(caQtDM_Lib PRIVATE advapi32)
endif()

caqtdm_set_output_dir(caQtDM_Lib)
caqtdm_apply_rpath(caQtDM_Lib)

install(TARGETS caQtDM_Lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
