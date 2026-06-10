include(../tst_plugins.pri)

SOURCES += tst_bsread.cpp \
    tst_bsread_decode.cpp \
    tst_bsread_dispatchercontrol.cpp

HEADERS += \
    fakenetworkreply.h \
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
    -lqtcontrols \
    -lcaQtDM_Lib \
    -L$(CAQTDM_COLLECT)/controlsystems \
    -lbsread_Plugin

_EPICSLIB = $$(EPICSLIB)
!isEmpty(_EPICSLIB) {
    QMAKE_RPATHDIR += $$(EPICSLIB)
    LIBS += -L$$(EPICSLIB) -lca -lCom
} else {
    QMAKE_RPATHDIR += $$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH)
    LIBS += -L$$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH) -lca -lCom
}

CONFIG += Define_ZMQ_Lib caqtdm_rpath
include(../../../../caQtDM.pri)

linux {
    QMAKE_LFLAGS += -Wl,--disable-new-dtags
}
