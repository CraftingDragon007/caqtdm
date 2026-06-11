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

#ifndef CONSOLELOGHANDLER_H
#define CONSOLELOGHANDLER_H

#include "abstractloghandler.h"

#include <QObject>

class ConsoleLogHandler : public QObject, public AbstractLogHandler
{
    Q_OBJECT
public:
    explicit ConsoleLogHandler(QObject *parent = Q_NULLPTR);
    ~ConsoleLogHandler() override;

    /**
     * @brief Prints the passed log to stdout or sterr, based on QtMsgType.
     * This function is not thread-safe.
     * @param log: The log to print
     */
    void handleLog(const Log &log) override;

public slots:
    /**
     * @brief Flushes stdout and stderr.
     * This function is not guaranteed to be thread-safe.
     */
    void flush() override;

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    bool m_flushEachLog;
    bool m_verboseOutput;

#ifdef Q_OS_WIN
    bool m_isDebuggerPresent;
#endif
};

#endif // CONSOLELOGHANDLER_H
