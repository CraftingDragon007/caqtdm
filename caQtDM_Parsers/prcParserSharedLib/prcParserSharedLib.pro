TARGET_PRODUCT = "PRC converter library for Display Manager"
TARGET_FILENAME = "prcParser.dll"

include(../../caQtDM_Viewer/qtdefs.pri)
CONFIG += caQtDM_xdl2ui
include(../../caQtDM.pri)

contains(QT_VER_MAJ, 5) {
  QT       += widgets uitools
  DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
}
contains(QT_VER_MAJ, 6) {
  QT       += widgets uitools
  DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
}

TEMPLATE = lib
CONFIG	+= shared plugin

DEFINES += PRCPARSER_MAKEDLL

INCLUDEPATH += .
INCLUDEPATH += ../prcParserSrc

MOC_DIR = moc
VPATH += ../prcParserSrc

RC_FILE = ../../caQtDM_Viewer/src/caQtDM.rc

HEADERS += parseprcfile.h \
    prctokenizer.h \
    prcuiwriter.h \
    prcparserdefs.h

SOURCES += parseprcfile.cpp \
    prctokenizer.cpp \
    prcuiwriter.cpp

TARGET = prcParser
