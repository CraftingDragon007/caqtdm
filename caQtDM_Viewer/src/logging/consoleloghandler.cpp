#include "consoleloghandler.h"

#include <iostream>

ConsoleLogHandler::ConsoleLogHandler(): m_flushEachLog(false) {}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log)
{
    std::cout << "[" << log.timestampUtc.toStdString() << "] " << log.loglevelString.toStdString() << " | " << log.locationString.toStdString() << "> " << log.message.toStdString() << "\n";

    if (m_flushEachLog)
        std::cout.flush();
}
