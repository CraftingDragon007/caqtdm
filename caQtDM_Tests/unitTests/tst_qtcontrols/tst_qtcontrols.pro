include(../unitTests.pri)
include (../../../caQtDM_Viewer/qtdefs.pri)

DEFINES += BUILDVERSION=\\\"UNITTEST\\\"

QT += testlib network gui widgets designer

CONFIG += qt console warn_on depend_includepath testcase moc build_always
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += tst_qtcontrols.cpp \
    tst_pvdialog.cpp

HEADERS += tst_pvdialog.h

# --- Tested classes below ---

HEADERS += ../../../caQtDM_QtControls/src/pvdialog.h

SOURCES += ../../../caQtDM_QtControls/src/pvdialog.cpp

INCLUDEPATH += ../../../caQtDM_QtControls/src \
    ../../../caQtDM_Lib/src \
    $$(QWTINCLUDE)

LIBS += \
    -L$$(CAQTDM_COLLECT) \
    -lqtcontrols

LIBS += -Wl,-rpath,$$(CAQTDM_COLLECT)
