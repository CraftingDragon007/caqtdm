# Control-system plugins (mirrors caQtDM_Plugins.pro and the per-plugin
# .pro files). demo/epics3/archive/environment/internal are always built,
# the rest follow the feature switches / auto-detection of qtdefs.pri.

set(_root "${CMAKE_CURRENT_SOURCE_DIR}")

# --------------------------------------------------------------------------------------------------
# Always built plugins
# --------------------------------------------------------------------------------------------------
caqtdm_add_cs_plugin(demo_plugin
    SOURCES demo/demo_plugin.cpp
    HEADERS demo/demo_plugin.h controlsinterface.h caQtDM_Plugins_global.h)

caqtdm_add_cs_plugin(internal_plugin
    SOURCES internal/internal_plugin.cpp internal/internal_channel.cpp
    HEADERS internal/internal_plugin.h internal/internal_channel.h controlsinterface.h caQtDM_Plugins_global.h)

caqtdm_add_cs_plugin(environment_plugin
    SOURCES environment/environment_plugin.cpp
    HEADERS environment/environment_plugin.h controlsinterface.h caQtDM_Plugins_global.h
    LINKS Qt6::Concurrent)

caqtdm_add_cs_plugin(epics3_plugin
    SOURCES epics3/epics3_plugin.cpp epics3/epicsSubs.c
    HEADERS epics3/epics3_plugin.h controlsinterface.h caQtDM_Plugins_global.h
    LINKS Epics::ca Epics::Com)

if(CAQTDM_EPICS7)
    set(EPICS4_LINKS Epics::pvAccess Epics::pvAccessCA Epics::pvData Epics::pvaClient Epics::nt)
    list(FILTER EPICS4_LINKS INCLUDE REGEX ".+")
    caqtdm_add_cs_plugin(epics4_plugin
        SOURCES epics4/epics4_plugin.cpp epics4/epics4_callbackThread.cpp epics4/epics4Requester.cpp
        HEADERS epics4/epics4_plugin.h epics4/epics4_callbackThread.h epics4/epics4Requester.h
            controlsinterface.h caQtDM_Plugins_global.h
        LINKS ${EPICS4_LINKS} Epics::ca Epics::Com
        DEFINES QT_NO_WARNING_OUTPUT QT_NO_DEBUG_OUTPUT)
endif()

# --------------------------------------------------------------------------------------------------
# Archive retrieval plugins (always on for desktop, like CONFIG += archive)
# --------------------------------------------------------------------------------------------------
if(CAQTDM_ARCHIVE_PLUGINS)
    caqtdm_add_cs_plugin(archiveSF_plugin
        SOURCES archive/archiveSF/archiveSF_plugin.cpp archive/archiveSF/sfRetrieval.cpp archive/archiverCommon.cpp
        HEADERS archive/archiveSF/archiveSF_plugin.h archive/archiveSF/sfRetrieval.h
            archive/archiverCommon.h controlsinterface.h caQtDM_Plugins_global.h
        INCLUDES "${_root}/archive" "${_root}/archive/archiveSF"
        LINKS Qt6::UiTools)

    caqtdm_add_cs_plugin(archiveHTTP_plugin
        SOURCES
            archive/archiveHTTP/archivehttp_plugin.cpp
            archive/archiveHTTP/httpretrieval.cpp
            archive/archiverGeneral.cpp
            archive/archiveHTTP/httpperformancedata.cpp
            archive/archiveHTTP/urlhandlerhttp.cpp
            archive/archiveHTTP/workerHttp.cpp
            archive/archiveHTTP/workerHttpThread.cpp
        HEADERS
            archive/archiveHTTP/archivehttp_plugin.h
            archive/archiveHTTP/httpretrieval.h
            archive/archiverGeneral.h
            archive/archiveHTTP/httpperformancedata.h
            archive/archiveHTTP/urlhandlerhttp.h
            archive/archiveHTTP/workerHttp.h
            archive/archiveHTTP/workerHttpThread.h
            controlsinterface.h caQtDM_Plugins_global.h
        INCLUDES "${_root}/archive" "${_root}/archive/archiveHTTP"
        LINKS Qt6::UiTools)
endif()

# --------------------------------------------------------------------------------------------------
# Feature-switch plugins
# --------------------------------------------------------------------------------------------------
if(CAQTDM_BUILD_MODBUS)
    # uses calcPerform/postfix from EPICS base (in Com)
    caqtdm_add_cs_plugin(modbus_plugin
        SOURCES modbus/modbus_Plugin.cpp modbus/modbus_decode.cpp modbus/modbus_channeldata.cpp
        HEADERS modbus/modbus_plugin.h modbus/modbus_decode.h modbus/modbus_channeldata.h
            controlsinterface.h caQtDM_Plugins_global.h
        LINKS Qt6::SerialBus Qt6::Concurrent Epics::Com)
endif()

if(CAQTDM_BUILD_GPS)
    caqtdm_add_cs_plugin(gps_plugin
        SOURCES gps/gps_plugin.cpp
        HEADERS gps/gps_plugin.h controlsinterface.h caQtDM_Plugins_global.h
        LINKS Qt6::Positioning)
endif()

if(CAQTDM_HAVE_OPCUA)
    set(OPCUA_SOURCES opcua/opcua_plugin.cpp opcua/opcua_core.cpp)
    set(OPCUA_HEADERS opcua/opcua_plugin.h opcua/opcua_core.h controlsinterface.h caQtDM_Plugins_global.h)
    set(OPCUA_FORMS "")
    if(CAQTDM_QT_OPCUA_X509)
        list(APPEND OPCUA_SOURCES opcua/certificatedialog.cpp opcua/x509certificate.cpp)
        list(APPEND OPCUA_HEADERS opcua/certificatedialog.h opcua/x509certificate.h)
        list(APPEND OPCUA_FORMS opcua/certificatedialog.ui)
    endif()
    caqtdm_add_cs_plugin(opcua_plugin
        SOURCES ${OPCUA_SOURCES}
        HEADERS ${OPCUA_HEADERS}
        FORMS ${OPCUA_FORMS}
        LINKS Qt6::OpcUa)
    if(CAQTDM_QT_OPCUA_X509)
        target_compile_definitions(opcua_plugin PRIVATE QT_OPCUA_X509)
    endif()
endif()

if(CAQTDM_HAVE_BSREAD)
    caqtdm_add_cs_plugin(bsread_Plugin
        SOURCES
            bsread/bsread_Plugin.cpp
            bsread/md5.cc
            bsread/bsread_decode.cpp
            bsread/bsread_channeldata.cpp
            bsread/bsread_dispatchercontrol.cpp
            bsread/bsread_wfhandling.cpp
            bsread/bsread_wfconverterthread.cpp
            bsread/bsread_internalchannel.cpp
        HEADERS
            bsread/bsread_Plugin.h
            bsread/bsread_decode.h
            bsread/bsread_channeldata.h
            bsread/bsread_dispatchercontrol.h
            bsread/bsread_wfhandling.h
            bsread/bsread_wfconverter.h
            bsread/bsread_wfblockconverter.h
            bsread/bsread_wfconverterthread.h
            bsread/bsread_internalchannel.h
            controlsinterface.h caQtDM_Plugins_global.h
        LINKS caqtdm::zmq Qt6::Concurrent)
endif()
