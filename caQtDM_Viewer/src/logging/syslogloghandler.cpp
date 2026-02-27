#include "syslogloghandler.h"

#include <syslog.h>

SyslogLogHandler::SyslogLogHandler() {
    openlog("caQtDM", LOG_PID | LOG_NDELAY, LOG_USER);
}
SyslogLogHandler::~SyslogLogHandler() {
    closelog();
}

void SyslogLogHandler::handleLog(const Log &log) {
    int priority;
    switch (log.loglevel) {
    case QtDebugMsg:
        priority = LOG_DEBUG;
        break;
    case QtInfoMsg:
        priority = LOG_INFO;
        break;
    case QtWarningMsg:
        priority = LOG_WARNING;
        break;
    case QtCriticalMsg:
        priority = LOG_CRIT;
        break;
    case QtFatalMsg:
    default:
        priority = LOG_EMERG;
        break;
    }

    syslog(priority, "%s", qUtf8Printable("[" + log.timestampUtc + "] " + log.loglevelString + " | " + log.locationString + "> " + log.message));
}
