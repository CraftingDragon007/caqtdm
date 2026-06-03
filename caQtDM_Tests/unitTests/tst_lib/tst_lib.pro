include(../../../caQtDM_Viewer/qtdefs.pri)
include(../unitTests.pri)

QT += network gui widgets designer uitools printsupport

SOURCES += tst_lib.cpp \
    tst_caqtdm_lib.cpp

HEADERS += tst_caqtdm_lib.h \
    fakefileopenwindow.h

# --- Tested classes below ---

HEADERS += ../../../caQtDM_Lib/src/caqtdm_lib.h

SOURCES += ../../../caQtDM_Lib/src/caqtdm_lib.cpp

INCLUDEPATH += ../../../caQtDM_QtControls/src \
    ../../../caQtDM_Lib \
    ../../../caQtDM_Lib/src \
    ../../../caQtDM_Lib/caQtDM_Plugins \
    ../../../caQtDM_Parsers/adlParserSrc \
    ../../../caQtDM_Parsers/edlParserSrc \
    $$(QWTINCLUDE) \
    $$(EPICSINCLUDE)

LIBS += \
    -L$$(CAQTDM_COLLECT) \
    -lcaQtDM_Lib \
    -lqtcontrols \
    -L$$(EPICS_BASE)/lib/$$(EPICS_HOST_ARCH) \
    -lca -lCOM \
    -L$$(QWTHOME)/lib \
    -l$$(QWTLIBNAME)
