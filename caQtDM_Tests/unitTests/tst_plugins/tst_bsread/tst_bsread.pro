include(../tst_plugins.pri)

SOURCES += tst_bsread.cpp \
    tst_bsread_decode.cpp \
    tst_bsread_dispatchercontrol.cpp

HEADERS += \
    tst_bsread_decode.h \
    tst_bsread_dispatchercontrol.h

# --- Tested classes below ---

HEADERS += ../../../../caQtDM_Lib/caQtDM_Plugins/bsread/bsread_dispatchercontrol.h \
    ../../../../caQtDM_Lib/caQtDM_Plugins/bsread/bsread_decode.h

SOURCES += ../../../../caQtDM_Lib/caQtDM_Plugins/bsread/bsread_dispatchercontrol.cpp \
    ../../../../caQtDM_Lib/caQtDM_Plugins/bsread/bsread_decode.cpp

INCLUDEPATH += ../../../../caQtDM_QtControls/src \
    ../../../../caQtDM_Lib/src \
    ../../../../caQtDM_Lib/caQtDM_Plugins \
    ../../../../caQtDM_Lib/caQtDM_Plugins/bsread \
    $$(EPICSINCLUDE) \
    $$(ZMQINC)

LIBS += \
    -L$$(CAQTDM_COLLECT) \
    -lcaQtDM_Lib \
    -lqtcontrols \
    -L$(CAQTDM_COLLECT)/controlsystems \
    -lbsread_Plugin

_EPICSLIB = $$(EPICSLIB)
!isEmpty(_EPICSLIB) {
    LIBS += -Wl,-rpath,$$(EPICSLIB)
    LIBS += -L$$(EPICSLIB) -lca -lCom
} else {
    LIBS += -Wl,-rpath,$$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH)
    LIBS += -L$$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH) -lca -lCom
}

LIBS += -Wl,-rpath,$$(ZMQLIB)

CONFIG += Define_ZMQ_Lib caqtdm_rpath
include(../../../../caQtDM.pri)
