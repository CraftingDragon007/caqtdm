include(../tst_plugins.pri)

SOURCES += tst_internal.cpp \
    tst_internal_channel.cpp \
    tst_internal_plugin.cpp

HEADERS += \
    tst_internal_channel.h \
    tst_internal_plugin.h

# --- Tested classes below ---

HEADERS += ../../../caQtDM_Plugins/internal/internal_channel.h \
    ../../../caQtDM_Plugins/internal/internal_plugin.h

SOURCES += ../../../caQtDM_Plugins/internal/internal_channel.cpp \
    ../../../caQtDM_Plugins/internal/internal_plugin.cpp

INCLUDEPATH += ../../../caQtDM_QtControls/src \
    ../../../caQtDM_Lib/src \
    ../../../caQtDM_Plugins \
    ../../../caQtDM_Plugins/internal \
    $$(EPICSINCLUDE)

LIBS += \
    -L$$(CAQTDM_COLLECT) \
    -lqtcontrols \
    -lcaQtDM_Lib

_EPICSLIB = $$(EPICSLIB)
!isEmpty(_EPICSLIB) {
    QMAKE_RPATHDIR += $$(EPICSLIB)
    LIBS += -L$$(EPICSLIB) -lca -lCom
} else {
    QMAKE_RPATHDIR += $$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH)
    LIBS += -L$$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH) -lca -lCom
}

CONFIG += caqtdm_rpath
include(../../../caQtDM.pri)

linux {
    QMAKE_LFLAGS += -Wl,--disable-new-dtags
}

macos {
    QMAKE_RPATHDIR += $$(QWTLIB)
}
