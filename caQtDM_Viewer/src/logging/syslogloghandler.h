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

#ifndef SYSLOGLOGHANDLER_H
#define SYSLOGLOGHANDLER_H

#include "abstractloghandler.h"

class SyslogLogHandler : public AbstractLogHandler
{
public:
    SyslogLogHandler();
    ~SyslogLogHandler() override;

    /**
     * @brief Calls syslog with log.
     * This function is thread-safe.
     * @param log: The log to process.
     */
    void handleLog(const Log &log) override;
    /**
     * @brief Does nothing.
     */
    void flush() override {};
};

#endif // SYSLOGLOGHANDLER_H
