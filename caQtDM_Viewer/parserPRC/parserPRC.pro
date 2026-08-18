TARGET_PRODUCT = "PRC converter for Display Manager"
TARGET_FILENAME = "prc2ui.exe"

include(../qtdefs.pri)
CONFIG += caQtDM_xdl2ui

include(../../caQtDM.pri)

contains(QT_VER_MAJ, 5) {
  QT       += widgets uitools
  DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
}
contains(QT_VER_MAJ, 6) {
  QT       += widgets uitools
}
TEMPLATE = app
INCLUDEPATH += .
MOC_DIR = moc
RC_FILE = ../../caQtDM_Viewer/src/caQtDM.rc

# Input
# CLI driver around libprcParser from caQtDM_Parsers (built there like
# the adl/edl parser libraries)
INCLUDEPATH += ../../caQtDM_Parsers/prcParserSrc
SOURCES += prcParser.cpp

unix {
    LIBS += $(CAQTDM_COLLECT)/libprcParser.a
}
win32 {
    win32-msvc* || msvc {
        LIBS += $$(CAQTDM_COLLECT)/prcParser.lib
    }
    win32-g++ {
        LIBS += $$(CAQTDM_COLLECT)/libprcParser.a
    }
}

TARGET = prc2ui

OTHER_FILES += \
    pepfeatures.prc
