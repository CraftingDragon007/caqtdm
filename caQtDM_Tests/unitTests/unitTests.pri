DEFINES += UNIT_TESTING
DEFINES += BUILDVERSION=\\\"UNITTEST\\\"

TEMPLATE = app

QT += testlib

CONFIG += qt console warn_on depend_includepath testcase moc build_always
CONFIG -= app_bundle

LIBS += -Wl,-rpath,$$(CAQTDM_COLLECT)
