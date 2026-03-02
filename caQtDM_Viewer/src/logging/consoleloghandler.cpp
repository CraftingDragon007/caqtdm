#include "consoleloghandler.h"

#include <iostream>

#define ENV_NO_FLUSH "CAQTDM_LOGGING_CONSOLE_NO_FLUSH"
#define ENV_VERBOSE_OUTPUT "CAQTDM_LOGGING_CONSOLE_VERBOSE"

ConsoleLogHandler::ConsoleLogHandler(QObject *parent)
    : QObject(parent)
{
    m_flushEachLog = qEnvironmentVariableIsEmpty(ENV_NO_FLUSH);
    m_verboseOutput = !qEnvironmentVariableIsEmpty(ENV_VERBOSE_OUTPUT);
}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log)
{
    if (m_verboseOutput) {
        std::cout << "[" << log.timestampUtc.toStdString() << "] " << log.category.toStdString()
                  << " | " << log.loglevelString.toStdString() << " | "
                  << log.locationString.toStdString() << "> " << log.message.toStdString() << "\n";
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
