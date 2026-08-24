# caQtDM viewer application (mirrors caQtDM_Viewer.pro) and the adl2ui /
# edl2ui / prc2ui converter tools (mirrors parser/, parserEDM/, parserPRC/).

set(_src "${CMAKE_CURRENT_SOURCE_DIR}/src")

# --------------------------------------------------------------------------------------------------
# Build stamps (mirror caQtDM.pri caQtDM_Viewer scope)
# --------------------------------------------------------------------------------------------------
string(TIMESTAMP CAQTDM_BUILDTIME "%H:%M")
string(TIMESTAMP CAQTDM_BUILDDATE "%d-%m-%Y")
string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" CAQTDM_BUILDARCH)

# --------------------------------------------------------------------------------------------------
# Sources
# --------------------------------------------------------------------------------------------------
set(VIEWER_SOURCES
    ${_src}/caQtDM.cpp
    ${_src}/fileopenwindow.cpp
    ${_src}/messagebox.cpp
    ${_src}/configDialog.cpp
    ${_src}/pipereader.cpp
    ${_src}/signalhandler.cpp)

set(VIEWER_HEADERS
    ${_src}/messagebox.h
    ${_src}/fileopenwindow.h
    ${_src}/configDialog.h
    ${_src}/pipereader.h
    ${_src}/signalhandler.h
    ${_src}/loggingcategories.h)

if(NOT CAQTDM_NO_CUSTOM_LOGHANDLER)
    list(APPEND VIEWER_SOURCES
        ${_src}/logging/consoleloghandler.cpp
        ${_src}/logging/fileloghandler.cpp
        ${_src}/logging/generalloghandler.cpp
        ${_src}/logging/logstashloghandler.cpp)
    list(APPEND VIEWER_HEADERS
        ${_src}/logging/abstractloghandler.h
        ${_src}/logging/consoleloghandler.h
        ${_src}/logging/fileloghandler.h
        ${_src}/logging/generalloghandler.h
        ${_src}/logging/logstashloghandler.h)
    if(UNIX)
        list(APPEND VIEWER_SOURCES ${_src}/logging/syslogloghandler.cpp)
        list(APPEND VIEWER_HEADERS ${_src}/logging/syslogloghandler.h)
    endif()
endif()

# --------------------------------------------------------------------------------------------------
# Target
# --------------------------------------------------------------------------------------------------
if(ANDROID)
    qt_add_executable(caQtDM ${VIEWER_SOURCES} ${VIEWER_HEADERS})
else()
    add_executable(caQtDM WIN32 ${VIEWER_SOURCES} ${VIEWER_HEADERS})
endif()

if(CAQTDM_NO_CUSTOM_LOGHANDLER)
    target_compile_definitions(caQtDM PRIVATE CAQTDM_NO_CUSTOM_LOGHANDLER)
endif()

set_target_properties(caQtDM PROPERTIES AUTOUIC_SEARCH_PATHS "${_src}")

target_include_directories(caQtDM PRIVATE
    "${_src}"
    "${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src"
    "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src"
    "${PROJECT_SOURCE_DIR}/caQtDM_Plugins")

target_sources(caQtDM PRIVATE
    "${_src}/main.ui"
    "${_src}/caQtDM.qrc")

target_link_libraries(caQtDM
    PRIVATE
        caQtDM_Lib
        qtcontrols
        Qt6::Core
        Qt6::Gui
        Qt6::Svg
        Qt6::UiTools
        Qt6::Network
        Qt6::OpenGL
        Qt6::Xml
        Qt6::PrintSupport)

if(CAQTDM_BUILD_MODBUS)
    target_link_libraries(caQtDM PRIVATE Qt6::SerialBus)
    target_compile_definitions(caQtDM PRIVATE CAQTDM_MODBUS)
endif()
if(CAQTDM_BUILD_GPS)
    target_link_libraries(caQtDM PRIVATE Qt6::Positioning)
    target_compile_definitions(caQtDM PRIVATE CAQTDM_GPS)
endif()
if(CAQTDM_HAVE_OPCUA)
    target_compile_definitions(caQtDM PRIVATE CAQTDM_OPCUA)
endif()
if(CAQTDM_EPICS7)
    target_compile_definitions(caQtDM PRIVATE EPICS4)
endif()

list(APPEND _caqtdm_viewer_defs
    SUPPORT=\"EPICS\"
    BUILDVERSION=\"${CAQTDM_VERSION_STR}\"
    BUILDARCH=\"${CAQTDM_BUILDARCH}\"
    BUILDTIME=\"${CAQTDM_BUILDTIME}\"
    BUILDDATE=\"${CAQTDM_BUILDDATE}\")
target_compile_definitions(caQtDM PRIVATE ${_caqtdm_viewer_defs})

if(MSVC)
    caqtdm_add_version_rc(caQtDM "${_src}/caQtDM.rc" "Display Manager" "caqtdm.exe")
    target_link_options(caQtDM PRIVATE /entry:mainCRTStartup /STACK:32000000)
endif()

if(APPLE)
    find_package(ZLIB REQUIRED)
    target_link_libraries(caQtDM PRIVATE ZLIB::ZLIB)
endif()

# --------------------------------------------------------------------------------------------------
# macOS app bundle (mirrors the macx scope in caQtDM.pri)
# --------------------------------------------------------------------------------------------------
if(APPLE AND NOT IOS)
    set_target_properties(caQtDM PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${_src}/Mac/Info.plist"
        MACOSX_BUNDLE_ICON_FILE "caQtDM.icns")

    set_source_files_properties("${_src}/caQtDM.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/lucida-sans-typewriter.ttf" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources/fonts)
    target_sources(caQtDM PRIVATE
        "${_src}/caQtDM.icns"
        "${CMAKE_CURRENT_SOURCE_DIR}/lucida-sans-typewriter.ttf")

    add_custom_command(TARGET caQtDM POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:caQtDM>/Contents/PlugIns/designer"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:caQtDM>/Contents/PlugIns/controlsystems"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:caQtDM>/Contents/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CAQTDM_COLLECT}/designer" "$<TARGET_BUNDLE_DIR:caQtDM>/Contents/PlugIns/designer"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CAQTDM_COLLECT}/controlsystems" "$<TARGET_BUNDLE_DIR:caQtDM>/Contents/PlugIns/controlsystems"
        COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_FILE:qtcontrols>" "$<TARGET_FILE:caQtDM_Lib>"
                "$<TARGET_BUNDLE_DIR:caQtDM>/Contents/Frameworks"
        COMMENT "Collecting plugins and frameworks into the caQtDM app bundle")
endif()

caqtdm_set_output_dir(caQtDM)
caqtdm_apply_rpath(caQtDM)

install(TARGETS caQtDM
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .)

# --------------------------------------------------------------------------------------------------
# Converter tools
# --------------------------------------------------------------------------------------------------
add_subdirectory(parser)
if(UNIX)
    add_subdirectory(parserEDM)
endif()
add_subdirectory(parserPRC)

install(TARGETS adl2ui prc2ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX)
    install(TARGETS edl2ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
