include (../../../caQtDM_Viewer/qtdefs.pri)
QT += core gui opcua
contains(QT_VER_MAJ, 5) {
    QT     += widgets
}
contains(QT_VER_MAJ, 6) {
    QT     += widgets
}

CONFIG += warn_on
CONFIG += release
CONFIG += opcua_plugin
CONFIG += c++13
include (../../../caQtDM.pri)

MOC_DIR = ./moc
VPATH += ./src

TEMPLATE        = lib
CONFIG         += plugin
INCLUDEPATH    += .
INCLUDEPATH    += ../
INCLUDEPATH    += ../../src
INCLUDEPATH    += ../../../caQtDM_QtControls/src

HEADERS         = ../controlsinterface.h \
    certificatedialog.h \
    opcua_plugin.h opcua_core.h \
    x509certificate.h
SOURCES         = \
    certificatedialog.cpp \
    opcua_plugin.cpp opcua_core.cpp \
    x509certificate.cpp
TARGET          = opcua_plugin
android {
   INCLUDEPATH += $(ANDROIDFUNCTIONSINCLUDE)
}

FORMS += \
    certificatedialog.ui
