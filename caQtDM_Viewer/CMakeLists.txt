set(_viewer_qt_components Core Gui Widgets Svg UiTools Network)
if(QT_VERSION_MAJOR EQUAL 5)
    list(APPEND _viewer_qt_components OpenGL)
else()
    list(APPEND _viewer_qt_components OpenGLWidgets)
endif()
if(NOT CAQTDM_ENABLE_MOBILE)
    list(APPEND _viewer_qt_components PrintSupport)
endif()
if(QT_VERSION_MAJOR EQUAL 5 AND UNIX AND NOT APPLE AND NOT ANDROID)
    list(APPEND _viewer_qt_components X11Extras)
endif()
if(TARGET modbus_plugin)
    list(APPEND _viewer_qt_components SerialBus)
endif()
if(TARGET gps_plugin)
    list(APPEND _viewer_qt_components Positioning)
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${_viewer_qt_components})

set(_viewer_sources
    src/caQtDM.cpp
    src/fileopenwindow.cpp
    src/messagebox.cpp
    src/configDialog.cpp
    src/pipereader.cpp)

set(_viewer_defs)
if(QT_VERSION_MAJOR EQUAL 5)
    list(APPEND _viewer_defs QT_DISABLE_DEPRECATED_BEFORE=0x000000)
else()
    list(APPEND _viewer_defs QT_DISABLE_DEPRECATED_BEFORE=0x050000)
endif()
if(CAQTDM_ENABLE_MODBUS_PLUGIN AND TARGET modbus_plugin)
    list(APPEND _viewer_defs CAQTDM_MODBUS)
endif()
if(CAQTDM_ENABLE_GPS_PLUGIN AND TARGET gps_plugin)
    list(APPEND _viewer_defs CAQTDM_GPS)
endif()
if(CAQTDM_ENABLE_EPICS4_PLUGIN)
    list(APPEND _viewer_defs EPICS4)
endif()
if(CAQTDM_ENABLE_MOBILE)
    if(CAQTDM_ENABLE_ARCHIVE_SF_PLUGIN)
        list(APPEND _viewer_defs ARCHIVESF)
    endif()
    if(CAQTDM_ENABLE_ARCHIVE_HIPA_PLUGIN)
        list(APPEND _viewer_defs ARCHIVEHIPA)
    endif()
    if(CAQTDM_ENABLE_ARCHIVE_PRO_PLUGIN)
        list(APPEND _viewer_defs ARCHIVEPRO)
    endif()
    if(CAQTDM_ENABLE_ARCHIVE_CA_PLUGIN)
        list(APPEND _viewer_defs ARCHIVECA)
    endif()
endif()

set(_viewer_exe_type)
if(WIN32)
    list(APPEND _viewer_exe_type WIN32)
endif()
if(APPLE)
    list(APPEND _viewer_exe_type MACOSX_BUNDLE)
endif()

add_executable(caqtdm
    ${_viewer_exe_type}
    ${_viewer_sources}
    src/main.ui
    src/caQtDM.qrc)

set_target_properties(caqtdm PROPERTIES
    OUTPUT_NAME "caQtDM"
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(caqtdm PRIVATE ${_viewer_defs})
caqtdm_target_version_info(caqtdm
    PRODUCT "Display Manager"
    FILENAME "caqtdm.exe")
caqtdm_target_build_info(caqtdm SUPPORT ${CAQTDM_SUPPORT_STRING})

if(WIN32)
    target_sources(caqtdm PRIVATE src/caQtDM.rc)
endif()
if(APPLE)
    target_sources(caqtdm PRIVATE src/caQtDM.icns)
    set_source_files_properties(src/caQtDM.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    set_target_properties(caqtdm PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Mac/Info.plist")
endif()
if(ANDROID)
    set_property(TARGET caqtdm PROPERTY ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Android")
endif()

target_include_directories(caqtdm
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src
        ${PROJECT_SOURCE_DIR}/caQtDM_Lib/src
        ${PROJECT_SOURCE_DIR}/caQtDM_Lib/caQtDM_Plugins
        ${CAQTDM_QWT_INCLUDE_DIR})
if(WIN32)
    target_include_directories(caqtdm PRIVATE
        ${CAQTDM_EPICS_INCLUDE_DIR}
        ${CAQTDM_EPICS_INCLUDE_DIR}/os/win32
        ${CAQTDM_EPICS_INCLUDE_DIR}/compiler/msvc)
endif()
if(CAQTDM_ANDROID_FUNCTIONS_INCLUDE)
    target_include_directories(caqtdm PRIVATE ${CAQTDM_ANDROID_FUNCTIONS_INCLUDE})
endif()

set(_viewer_links
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::UiTools
    Qt${QT_VERSION_MAJOR}::Network
    caqtdm::lib
    caqtdm::qtcontrols
    caqtdm::qwt)
if(QT_VERSION_MAJOR EQUAL 5)
    list(APPEND _viewer_links Qt5::OpenGL)
else()
    list(APPEND _viewer_links Qt6::OpenGLWidgets)
endif()
if(NOT CAQTDM_ENABLE_MOBILE)
    list(APPEND _viewer_links Qt${QT_VERSION_MAJOR}::PrintSupport)
endif()
if(QT_VERSION_MAJOR EQUAL 5 AND UNIX AND NOT APPLE AND NOT ANDROID)
    list(APPEND _viewer_links Qt5::X11Extras)
endif()
if(TARGET modbus_plugin)
    list(APPEND _viewer_links Qt${QT_VERSION_MAJOR}::SerialBus)
endif()
if(TARGET gps_plugin)
    list(APPEND _viewer_links Qt${QT_VERSION_MAJOR}::Positioning)
endif()
if(CAQTDM_QT_CORE5COMPAT_TARGET)
    list(APPEND _viewer_links ${CAQTDM_QT_CORE5COMPAT_TARGET})
endif()

target_link_libraries(caqtdm PRIVATE ${_viewer_links})
if(ANDROID AND TARGET caqtdm::android_functions)
    target_link_libraries(caqtdm PRIVATE caqtdm::android_functions)
endif()

install(TARGETS caqtdm
    RUNTIME DESTINATION "${CAQTDM_INSTALL_BINDIR}")

if(CAQTDM_ENABLE_ADL_EDL)
    if(TARGET caqtdm::adlparser)
        target_link_libraries(caqtdm PRIVATE caqtdm::adlparser)
    endif()
    if(TARGET caqtdm::edlparser)
        target_link_libraries(caqtdm PRIVATE caqtdm::edlparser)
    endif()
endif()

if(CAQTDM_ENABLE_MOBILE)
    set(_viewer_static_plugins
        qtcontrols_controllers_plugin
        qtcontrols_graphics_plugin
        qtcontrols_monitors_plugin
        qtcontrols_utilities_plugin
        demo_plugin
        epics3_plugin
        environment_plugin)
    if(TARGET modbus_plugin)
        list(APPEND _viewer_static_plugins modbus_plugin)
    endif()
    if(TARGET gps_plugin)
        list(APPEND _viewer_static_plugins gps_plugin)
    endif()
    if(TARGET epics4_plugin)
        list(APPEND _viewer_static_plugins epics4_plugin)
    endif()
    if(TARGET archiveSF_plugin)
        list(APPEND _viewer_static_plugins archiveSF_plugin)
    endif()
    if(TARGET archiveHIPA_plugin)
        list(APPEND _viewer_static_plugins archiveHIPA_plugin)
    endif()
    if(TARGET archivePRO_plugin)
        list(APPEND _viewer_static_plugins archivePRO_plugin)
    endif()
    if(TARGET archiveCA_plugin)
        list(APPEND _viewer_static_plugins archiveCA_plugin)
    endif()
    foreach(_plugin_target IN LISTS _viewer_static_plugins)
        if(TARGET ${_plugin_target})
            target_link_libraries(caqtdm PRIVATE ${_plugin_target})
        endif()
    endforeach()
endif()
