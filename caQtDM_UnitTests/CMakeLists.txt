# Unit tests (mirrors caQtDM_UnitTests.pro and unitTests.pri).
# All tests define UNIT_TESTING; the ones testing caqtdm_lib compile
# caqtdm_lib.cpp themselves because under UNIT_TESTING the class is renamed
# to CaQtDM_Lib_TEST, which the prebuilt library does not export.

add_library(caqtdm_test_common INTERFACE)
target_compile_definitions(caqtdm_test_common INTERFACE
    UNIT_TESTING
    BUILDVERSION=\"UNITTEST\"
    QT_DISABLE_DEPRECATED_BEFORE=0x050000)
target_link_libraries(caqtdm_test_common INTERFACE Qt6::Test)
if(MSVC)
    target_compile_options(caqtdm_test_common INTERFACE /wd4005)
endif()

set(_tests_src "${PROJECT_SOURCE_DIR}/caQtDM_QtControls/src")
set(_lib_src "${PROJECT_SOURCE_DIR}/caQtDM_Lib/src")
set(_viewer_src "${PROJECT_SOURCE_DIR}/caQtDM_Viewer/src")
set(_plugins_root "${PROJECT_SOURCE_DIR}/caQtDM_Plugins")

# --------------------------------------------------------------------------------------------------
# tst_viewer: logging system tests (no GUI)
# --------------------------------------------------------------------------------------------------
add_executable(tst_viewer
    tst_viewer/tst_viewer.cpp
    tst_viewer/tst_logging/tst_consoleloghandler.cpp
    tst_viewer/tst_logging/tst_fileloghandler.cpp
    tst_viewer/tst_logging/tst_generalloghandler.cpp
    tst_viewer/tst_logging/tst_logstashloghandler.cpp
    "${_viewer_src}/logging/consoleloghandler.cpp"
    "${_viewer_src}/logging/fileloghandler.cpp"
    "${_viewer_src}/logging/generalloghandler.cpp"
    "${_viewer_src}/logging/logstashloghandler.cpp"
    "${_viewer_src}/logging/abstractloghandler.h"
    "${_viewer_src}/logging/consoleloghandler.h"
    "${_viewer_src}/logging/fileloghandler.h"
    "${_viewer_src}/logging/generalloghandler.h"
    "${_viewer_src}/logging/logstashloghandler.h"
    tst_viewer/tst_logging/tst_consoleloghandler.h
    tst_viewer/tst_logging/tst_fileloghandler.h
    tst_viewer/tst_logging/tst_generalloghandler.h
    tst_viewer/tst_logging/tst_logstashloghandler.h)
if(UNIX)
    list(APPEND _tst_viewer_extra
        "${_viewer_src}/logging/syslogloghandler.cpp"
        "${_viewer_src}/logging/syslogloghandler.h")
    target_sources(tst_viewer PRIVATE ${_tst_viewer_extra})
endif()
target_include_directories(tst_viewer PRIVATE "${_viewer_src}/logging")
target_link_libraries(tst_viewer PRIVATE caqtdm_test_common Qt6::Network)

# --------------------------------------------------------------------------------------------------
# tst_qtcontrols: widget tests (pvdialog compiled in, links qtcontrols)
# --------------------------------------------------------------------------------------------------
add_executable(tst_qtcontrols
    tst_qtcontrols/tst_qtcontrols.cpp
    tst_qtcontrols/tst_pvdialog.cpp
    tst_qtcontrols/tst_gensoftpv.cpp
    tst_qtcontrols/tst_canumeric.cpp
    tst_qtcontrols/tst_caspinbox.cpp
    tst_qtcontrols/tst_caapplynumeric.cpp
    "${_tests_src}/pvdialog.cpp"
    tst_qtcontrols/tst_pvdialog.h
    tst_qtcontrols/tst_gensoftpv.h
    tst_qtcontrols/tst_canumeric.h
    tst_qtcontrols/tst_caspinbox.h
    tst_qtcontrols/tst_caapplynumeric.h
    tst_qtcontrols/tst_numeric_suite.h
    tst_qtcontrols/fakeformwindow.h
    "${_tests_src}/pvdialog.h")
target_include_directories(tst_qtcontrols PRIVATE
    "${_tests_src}"
    "${_lib_src}")
target_link_libraries(tst_qtcontrols PRIVATE
    caqtdm_test_common
    qtcontrols
    Qt6::Network
    Qt6::Gui
    Qt6::Widgets
    Qt6::Designer)

# --------------------------------------------------------------------------------------------------
# tst_lib: dispatcher tests (caqtdm_lib.cpp + internal plugin compiled in)
# --------------------------------------------------------------------------------------------------
add_executable(tst_lib
    tst_lib/tst_lib.cpp
    tst_lib/tst_caqtdm_lib.cpp
    "${_lib_src}/caqtdm_lib.cpp"
    "${_plugins_root}/internal/internal_channel.cpp"
    "${_plugins_root}/internal/internal_plugin.cpp"
    tst_lib/tst_caqtdm_lib.h
    tst_lib/fakefileopenwindow.h
    "${_lib_src}/caqtdm_lib.h"
    "${_plugins_root}/internal/internal_channel.h"
    "${_plugins_root}/internal/internal_plugin.h"
    "${_viewer_src}/main.ui")
set_target_properties(tst_lib PROPERTIES AUTOUIC_SEARCH_PATHS "${_viewer_src}")
target_include_directories(tst_lib PRIVATE
    "${_tests_src}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Lib"
    "${_lib_src}"
    "${_plugins_root}"
    "${_plugins_root}/internal"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc")
target_link_libraries(tst_lib PRIVATE
    caqtdm_test_common
    caQtDM_Lib
    qtcontrols
    Epics::ca
    Epics::Com
    Qt6::Network
    Qt6::Gui
    Qt6::Widgets
    Qt6::Designer
    Qt6::UiTools
    Qt6::PrintSupport)

# --------------------------------------------------------------------------------------------------
# tst_viewer_input: viewer input handling (viewer sources + caqtdm_lib compiled in)
# --------------------------------------------------------------------------------------------------
add_executable(tst_viewer_input
    tst_viewer_input/tst_viewer_input.cpp
    tst_viewer_input/tst_envfilter.cpp
    tst_viewer_input/tst_networkaccess.cpp
    "${_viewer_src}/fileopenwindow.cpp"
    "${_viewer_src}/messagebox.cpp"
    "${_viewer_src}/configDialog.cpp"
    "${_tests_src}/networkaccess.cpp"
    "${_lib_src}/caqtdm_lib.cpp"
    tst_viewer_input/tst_envfilter.h
    tst_viewer_input/tst_networkaccess.h
    helpers/fakenetworkaccessmanager.h
    helpers/fakenetworkreply.h
    "${_viewer_src}/fileopenwindow.h"
    "${_viewer_src}/messagebox.h"
    "${_viewer_src}/configDialog.h"
    "${_viewer_src}/loggingcategories.h"
    "${_tests_src}/networkaccess.h"
    "${_lib_src}/caqtdm_lib.h"
    "${_viewer_src}/main.ui")
set_target_properties(tst_viewer_input PROPERTIES AUTOUIC_SEARCH_PATHS "${_viewer_src}")
target_include_directories(tst_viewer_input PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/helpers"
    "${_viewer_src}"
    "${_tests_src}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Lib"
    "${_lib_src}"
    "${_plugins_root}"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/adlParserSrc"
    "${PROJECT_SOURCE_DIR}/caQtDM_Parsers/edlParserSrc")
target_compile_definitions(tst_viewer_input PRIVATE
    SUPPORT=\"EPICS\"
    BUILDARCH=\"UNITTEST\"
    BUILDTIME=\"\"
    BUILDDATE=\"\")
target_link_libraries(tst_viewer_input PRIVATE
    caqtdm_test_common
    caQtDM_Lib
    qtcontrols
    Epics::ca
    Epics::Com
    Qt6::Network
    Qt6::Gui
    Qt6::Widgets
    Qt6::Designer
    Qt6::UiTools
    Qt6::PrintSupport)

# --------------------------------------------------------------------------------------------------
# Plugin tests
# --------------------------------------------------------------------------------------------------
add_executable(tst_internal
    tst_plugins/tst_internal/tst_internal.cpp
    tst_plugins/tst_internal/tst_internal_channel.cpp
    tst_plugins/tst_internal/tst_internal_plugin.cpp
    "${_plugins_root}/internal/internal_channel.cpp"
    "${_plugins_root}/internal/internal_plugin.cpp"
    tst_plugins/tst_internal/tst_internal_channel.h
    tst_plugins/tst_internal/tst_internal_plugin.h
    "${_plugins_root}/internal/internal_channel.h"
    "${_plugins_root}/internal/internal_plugin.h")
target_include_directories(tst_internal PRIVATE
    "${_tests_src}"
    "${_lib_src}"
    "${_plugins_root}"
    "${_plugins_root}/internal")
target_link_libraries(tst_internal PRIVATE
    caqtdm_test_common
    qtcontrols
    caQtDM_Lib
    Epics::ca
    Epics::Com
    Qt6::Network
    Qt6::Gui
    Qt6::Concurrent
    Qt6::Widgets
    Qt6::Designer
    Qt6::UiTools
    Qt6::PrintSupport)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(tst_internal PRIVATE "LINKER:--disable-new-dtags")
endif()

if(CAQTDM_HAVE_BSREAD)
    add_executable(tst_bsread
        tst_plugins/tst_bsread/tst_bsread.cpp
        tst_plugins/tst_bsread/tst_bsread_decode.cpp
        tst_plugins/tst_bsread/tst_bsread_dispatchercontrol.cpp
        "${_plugins_root}/bsread/bsread_dispatchercontrol.cpp"
        "${_plugins_root}/bsread/bsread_decode.cpp"
        tst_plugins/tst_bsread/fakenetworkreply.h
        tst_plugins/tst_bsread/tst_bsread_decode.h
        tst_plugins/tst_bsread/tst_bsread_dispatchercontrol.h
        "${_plugins_root}/bsread/bsread_dispatchercontrol.h"
        "${_plugins_root}/bsread/bsread_decode.h")
    target_include_directories(tst_bsread PRIVATE
        "${_tests_src}"
        "${_lib_src}"
        "${_plugins_root}"
        "${_plugins_root}/bsread"
        "${ZMQ_INCLUDE_DIR}")
    target_link_libraries(tst_bsread PRIVATE
        caqtdm_test_common
        qtcontrols
        caQtDM_Lib
        caqtdm::zmq
        Epics::ca
        Epics::Com
        Qt6::Network
        Qt6::Gui
        Qt6::Concurrent
        Qt6::Widgets
        Qt6::Designer
        Qt6::UiTools
        Qt6::PrintSupport)
    target_link_libraries(tst_bsread PRIVATE "$<TARGET_FILE:bsread_Plugin>")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(tst_bsread PRIVATE "LINKER:--disable-new-dtags")
    endif()
endif()

# --------------------------------------------------------------------------------------------------
# CTest registration
# --------------------------------------------------------------------------------------------------
set(_all_tests tst_viewer tst_qtcontrols tst_lib tst_viewer_input tst_internal)
if(TARGET tst_bsread)
    list(APPEND _all_tests tst_bsread)
endif()

if(WIN32)
    # ctest's ENVIRONMENT property replaces the variable, and ';' inside a
    # value must be escaped — otherwise the PATH falls apart. Compose a
    # complete PATH: collect dirs, Qt bin, Qwt lib, EPICS bin, then whatever
    # PATH the configure shell had.
    get_filename_component(_qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
    set(_path "${CAQTDM_COLLECT}\\;${CAQTDM_COLLECT}/designer\\;${CAQTDM_COLLECT}/controlsystems\\;${_qt_bin_dir}")
    if(TARGET Qwt::Qwt)
        get_target_property(_qwt_location Qwt::Qwt IMPORTED_LOCATION_RELEASE)
        if(NOT _qwt_location)
            get_target_property(_qwt_location Qwt::Qwt IMPORTED_LOCATION)
        endif()
        if(_qwt_location)
            get_filename_component(_qwt_dir "${_qwt_location}" DIRECTORY)
            string(APPEND _path "\\;${_qwt_dir}")
        endif()
        unset(_qwt_location)
    endif()
    if(CAQTDM_EPICS_HOST_ARCH)
        string(APPEND _path "\\;${CAQTDM_EPICS_BASE}/bin/${CAQTDM_EPICS_HOST_ARCH}")
    endif()
    # the configure shell's PATH must be escaped as well, or ctest splits the
    # ENVIRONMENT list at its semicolons and truncates PATH
    string(REPLACE ";" "\\;" _env_path "$ENV{PATH}")
    string(APPEND _path "\\;${_env_path}")
    set(_test_env "QT_QPA_PLATFORM=offscreen;PATH=${_path}")
else()
    set(_test_env "QT_QPA_PLATFORM=offscreen;LD_LIBRARY_PATH=${CAQTDM_COLLECT}:${CAQTDM_COLLECT}/designer:${CAQTDM_COLLECT}/controlsystems")
endif()

foreach(_test ${_all_tests})
    add_test(NAME ${_test} COMMAND ${_test})
    set_tests_properties(${_test} PROPERTIES
        ENVIRONMENT "${_test_env}")
endforeach()

caqtdm_apply_rpath(tst_viewer)
caqtdm_apply_rpath(tst_qtcontrols)
caqtdm_apply_rpath(tst_lib)
caqtdm_apply_rpath(tst_viewer_input)
caqtdm_apply_rpath(tst_internal)
if(TARGET tst_bsread)
    caqtdm_apply_rpath(tst_bsread)
endif()
