include (../../caQtDM_Viewer/qtdefs.pri)
include(../unitTests.pri)

QT += network gui widgets designer

SOURCES += tst_qtcontrols.cpp \
    tst_pvdialog.cpp

HEADERS += tst_pvdialog.h \
    fakeformwindow.h

# --- Tested classes below ---

HEADERS += ../../caQtDM_QtControls/src/pvdialog.h

SOURCES += ../../caQtDM_QtControls/src/pvdialog.cpp

INCLUDEPATH += ../../caQtDM_QtControls/src \
    ../../caQtDM_Lib/src \
    $$(QWTINCLUDE)

LIBS += \
    -L$$(CAQTDM_COLLECT) \
    -lqtcontrols

macx {
    LIBS += -lz
    LIBS += -F$$(QWTLIB) -framework $$(QWTLIBNAME)
}
