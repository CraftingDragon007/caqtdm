include(../../../caQtDM_Viewer/qtdefs.pri)
include(../unitTests.pri)

QT += network gui concurrent
QT += widgets uitools printsupport designer

LIBS += -Wl,-rpath,$$(CAQTDM_COLLECT)/controlsystems
