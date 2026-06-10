include(../unitTests.pri)

QT += network
QT -= gui

SOURCES +=  tst_viewer.cpp \
    tst_logging/tst_consoleloghandler.cpp \
    tst_logging/tst_fileloghandler.cpp \
    tst_logging/tst_generalloghandler.cpp \
    tst_logging/tst_logstashloghandler.cpp

HEADERS += \
    tst_logging/tst_consoleloghandler.h \
    tst_logging/tst_fileloghandler.h \
    tst_logging/tst_generalloghandler.h \
    tst_logging/tst_logstashloghandler.h

# --- Tested classes below ---

SOURCES += \
    ../../../caQtDM_Viewer/src/logging/consoleloghandler.cpp \
    ../../../caQtDM_Viewer/src/logging/fileloghandler.cpp \
    ../../../caQtDM_Viewer/src/logging/generalloghandler.cpp \
    ../../../caQtDM_Viewer/src/logging/logstashloghandler.cpp

HEADERS += \
    ../../../caQtDM_Viewer/src/logging/abstractloghandler.h \
    ../../../caQtDM_Viewer/src/logging/consoleloghandler.h \
    ../../../caQtDM_Viewer/src/logging/fileloghandler.h \
    ../../../caQtDM_Viewer/src/logging/generalloghandler.h \
    ../../../caQtDM_Viewer/src/logging/logstashloghandler.h

unix {
    SOURCES += ../../../caQtDM_Viewer/src/logging/syslogloghandler.cpp
    HEADERS += ../../../caQtDM_Viewer/src/logging/syslogloghandler.h
}

INCLUDEPATH += ../../../caQtDM_Viewer/src/logging
