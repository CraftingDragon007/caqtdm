include(../../caQtDM_Viewer/qtdefs.pri)
include(../unitTests.pri)

QT += network gui concurrent
QT += widgets uitools printsupport designer

QMAKE_RPATHDIR += $$(CAQTDM_COLLECT)/controlsystems
