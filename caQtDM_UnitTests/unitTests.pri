DEFINES += UNIT_TESTING
DEFINES += BUILDVERSION=\\\"UNITTEST\\\"

TEMPLATE = app

QT += testlib

CONFIG += qt console warn_on depend_includepath testcase moc build_always
CONFIG -= app_bundle

QMAKE_RPATHDIR += $$(CAQTDM_COLLECT)

contains(QT_VER_MAJ, 5) {
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x000000
}

contains(QT_VER_MAJ, 6) {
    DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050000
}
