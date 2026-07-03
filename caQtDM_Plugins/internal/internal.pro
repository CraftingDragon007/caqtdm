include (../../caQtDM_Viewer/qtdefs.pri)
QT += core gui
contains(QT_VER_MAJ, 5) {
    QT     += widgets
}
contains(QT_VER_MAJ, 6) {
    QT     += widgets
}

CONFIG += warn_on
CONFIG += release
CONFIG += internal_plugin
include (../../caQtDM.pri)

MOC_DIR = ./moc
VPATH += ./src

TEMPLATE        = lib
CONFIG         += plugin
INCLUDEPATH    += .
INCLUDEPATH    += ../
INCLUDEPATH    += ../../caQtDM_Lib/src
INCLUDEPATH    += ../../caQtDM_QtControls/src
HEADERS         = internal_plugin.h internal_channel.h ../controlsinterface.h ../caQtDM_Plugins_global.h
SOURCES         = internal_plugin.cpp internal_channel.cpp
TARGET          = internal_plugin
