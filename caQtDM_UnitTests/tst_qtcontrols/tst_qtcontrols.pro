include (../../caQtDM_Viewer/qtdefs.pri)
include(../unitTests.pri)

QT += network gui widgets designer

SOURCES += tst_qtcontrols.cpp \
    tst_pvdialog.cpp

HEADERS += tst_pvdialog.h \
    fakeformwindow.h

greaterThan(QT_VER_MAJ, 5) {
    SOURCES += tst_ca3dconfig.cpp \
        tst_ca3dconfigdialog.cpp
    HEADERS += tst_ca3dconfig.h \
        tst_ca3dconfigdialog.h
}

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
    QMAKE_RPATHDIR += $$(QWTLIB)
}
