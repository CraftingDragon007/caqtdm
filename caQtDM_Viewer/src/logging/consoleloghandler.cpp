#include "consoleloghandler.h"

#include <iostream>

ConsoleLogHandler::ConsoleLogHandler()
{
    m_flushEachLog = false;
    m_verboseOutput = true;
}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log)
{
    if (m_verboseOutput) {
        std::cout << "[" << log.timestampUtc.toStdString() << "] " << log.loglevelString.toStdString()
                  << " | " << log.locationString.toStdString() << "> " << log.message.toStdString()
                  << "\n";
    } else {
        std::cout << log.message.toStdString() << "\n";
    }

    if (m_flushEachLog)
        std::cout.flush();
}
