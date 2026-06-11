/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2026
 *
 *  Author:
 *    Erik Schwarz
 *  Contact details:
 *    erik.schwarz@psi.ch
 */

#include "syslogloghandler.h"

#include <syslog.h>

SyslogLogHandler::SyslogLogHandler()
{
    openlog("caQtDM", LOG_PID | LOG_NDELAY, LOG_USER);
}
SyslogLogHandler::~SyslogLogHandler()
{
    closelog();
}

void SyslogLogHandler::handleLog(const Log &log)
{
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

    syslog(priority,
           "%s",
           qUtf8Printable("[" + log.timestampUtc + "] " + log.category + " | " + log.loglevelString
                          + " | " + log.locationString + "> " + log.message));
}
