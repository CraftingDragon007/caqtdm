#include "consoleloghandler.h"

#include <iostream>

ConsoleLogHandler::ConsoleLogHandler(QObject *parent)
    : QObject(parent)
{
    m_flushEachLog = true;
    m_verboseOutput = true;
}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log)
{
    if (m_verboseOutput) {
        std::cout << "[" << log.timestampUtc.toStdString() << "] " << log.category.toStdString() << " | "
                  << log.loglevelString.toStdString() << " | " << log.locationString.toStdString()
                  << "> " << log.message.toStdString() << "\n";
    } else {
        std::cout << log.message.toStdString() << "\n";
    }

    if (m_flushEachLog)
        std::cout.flush();
}

void ConsoleLogHandler::flush()
{
    std::cout.flush();
}
