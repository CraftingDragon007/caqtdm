include(../../caQtDM_Viewer/qtdefs.pri)
CONFIG += caQtDM_xdl2ui
include(../../caQtDM.pri)

contains(QT_VER_MAJ, 5) {
  QT       += widgets uitools
  DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
}
contains(QT_VER_MAJ, 6) {
  QT       += widgets uitools core
}

TEMPLATE = lib
CONFIG	+= static

INCLUDEPATH += .
INCLUDEPATH += ../prcParserSrc

MOC_DIR = moc
VPATH += ../prcParserSrc

HEADERS += parseprcfile.h \
    prctokenizer.h \
    prcuiwriter.h \
    prcparserdefs.h

SOURCES += parseprcfile.cpp \
    prctokenizer.cpp \
    prcuiwriter.cpp

TARGET = prcParser
