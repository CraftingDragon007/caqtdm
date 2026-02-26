#include "consoleloghandler.h"

#include <iostream>

ConsoleLogHandler::ConsoleLogHandler(): m_flushEachLog(false) {}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log)
{

    const QString locationString = log.file + ":" + log.function + ":"
                                   + QString::number(log.line);

    std::cout << "[" << log.timestampUtc.toStdString() << "] " << log.loglevelString.toStdString() << " | " << locationString.toStdString() << "> " << log.message.toStdString() << "\n";

    if (m_flushEachLog)
        std::cout.flush();
}
