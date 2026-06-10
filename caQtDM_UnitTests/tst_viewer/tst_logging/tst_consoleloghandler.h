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

#ifndef TST_CONSOLELOGHANDLER_H
#define TST_CONSOLELOGHANDLER_H

#include <sstream>

#include <QObject>

class TestConsoleLogHandler : public QObject
{
    Q_OBJECT
public:
    TestConsoleLogHandler() = default;

private:
    class CustomCoutBuffer : public std::stringbuf
    {
    public:
        int flushedCounter = 0;

    protected:
        int sync() override
        {
            flushedCounter++;
            return std::stringbuf::sync();
        }
    };

    CustomCoutBuffer *m_buffer;
    std::streambuf *m_originalCout;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void parametersInitializedFromEnv();
    void verboseOutputToggleWorks();
    void flushToggleWorks();
};

#endif // TST_CONSOLELOGHANDLER_H
