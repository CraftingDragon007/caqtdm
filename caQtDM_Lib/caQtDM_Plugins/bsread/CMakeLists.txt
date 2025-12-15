if(NOT CAQTDM_ZMQ_LIBRARY)
    message(FATAL_ERROR "ZeroMQ library path not configured. Set CAQTDM_ZMQ_LIBRARY or disable CAQTDM_ENABLE_BSREAD_PLUGIN.")
endif()

caqtdm_add_plugin(bsread_Plugin
    SOURCES
        bsread_Plugin.cpp
        md5.cc
        bsread_decode.cpp
        bsread_channeldata.cpp
        bsread_dispatchercontrol.cpp
        bsread_wfhandling.cpp
        bsread_wfconverterthread.cpp
        bsread_internalchannel.cpp
    QT_COMPONENTS
        Network
        Concurrent
    EXTRA_INCLUDES
        ${CAQTDM_ZMQ_INCLUDE_DIR}
    EXTRA_LIBS
        ${CAQTDM_ZMQ_LIBRARY})
