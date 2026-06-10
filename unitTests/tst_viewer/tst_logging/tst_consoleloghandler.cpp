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

#include "tst_consoleloghandler.h"

#include "consoleloghandler.h"

#include <iostream>

#include <QTest>

#define ENV_NO_FLUSH "CAQTDM_LOGGING_CONSOLE_NO_FLUSH"
#define ENV_VERBOSE_OUTPUT "CAQTDM_LOGGING_CONSOLE_VERBOSE"

void TestConsoleLogHandler::initTestCase()
{
    // code to be executed before the first test function

    m_buffer = new CustomCoutBuffer();
    m_originalCout = std::cout.rdbuf(m_buffer);
}

void TestConsoleLogHandler::init()
{
    // code to be executed before each test function

    qunsetenv(ENV_NO_FLUSH);
    qunsetenv(ENV_VERBOSE_OUTPUT);
    m_buffer->str("");
    m_buffer->flushedCounter = 0;
}

void TestConsoleLogHandler::cleanupTestCase()
{
    // code to be executed after the last test function

    std::cout.rdbuf(m_originalCout);
    delete m_buffer;
}

void TestConsoleLogHandler::cleanup()
{
    // code to be executed after each test function
}

void TestConsoleLogHandler::parametersInitializedFromEnv()
{
    // Default case
    auto *handler = new ConsoleLogHandler();
    QCOMPARE(handler->m_flushEachLog, true);
    QCOMPARE(handler->m_verboseOutput, false);

    // Set both
    qputenv(ENV_NO_FLUSH, "1");
    qputenv(ENV_VERBOSE_OUTPUT, "1");
    delete handler;
    handler = new ConsoleLogHandler();
    QCOMPARE(handler->m_flushEachLog, false);
    QCOMPARE(handler->m_verboseOutput, true);

    // Make both true (with empty var)
    qputenv(ENV_NO_FLUSH, "");
    delete handler;
    handler = new ConsoleLogHandler();
    QCOMPARE(handler->m_flushEachLog, true);
    QCOMPARE(handler->m_verboseOutput, true);

    // Make both false (with empty var)
    qputenv(ENV_NO_FLUSH, "1");
    qputenv(ENV_VERBOSE_OUTPUT, "");
    delete handler;
    handler = new ConsoleLogHandler();
    QCOMPARE(handler->m_flushEachLog, false);
    QCOMPARE(handler->m_verboseOutput, false);
}

void TestConsoleLogHandler::verboseOutputToggleWorks()
{
    ConsoleLogHandler handler;

    Log log;
    log.message = "message";
    log.timestampUtc = "timestamp";

    // test verbose output to contain both message and timestamp
    handler.m_verboseOutput = true;
    handler.handleLog(log);
    QString output = QString::fromStdString(m_buffer->str());
    QVERIFY(output.contains("message"));
    QVERIFY(output.contains("timestamp"));

    // test non-verbose output to only contain message, not timestamp
    handler.m_verboseOutput = false;
    m_buffer->str("");
    handler.handleLog(log);
    output = QString::fromStdString(m_buffer->str());
    QVERIFY(output.contains("message"));
    QVERIFY(!output.contains("timestamp"));
}

void TestConsoleLogHandler::flushToggleWorks()
{
    ConsoleLogHandler handler;

    Log log;
    log.message = "message";

    // test flushing disabled does not flush
    handler.m_flushEachLog = false;
    handler.handleLog(log);
    QString output = QString::fromStdString(m_buffer->str());
    QVERIFY(output.contains("message"));
    QCOMPARE(m_buffer->flushedCounter, 0);

    // test flushing enabled does flush
    handler.m_flushEachLog = true;
    m_buffer->str("");
    handler.handleLog(log);
    output = QString::fromStdString(m_buffer->str());
    QVERIFY(output.contains("message"));
    QCOMPARE(m_buffer->flushedCounter, 1);
}
