#include "consoleloghandler.h"

#include <iostream>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#define ENV_NO_FLUSH "CAQTDM_LOGGING_CONSOLE_NO_FLUSH"
#define ENV_VERBOSE_OUTPUT "CAQTDM_LOGGING_CONSOLE_VERBOSE"

ConsoleLogHandler::ConsoleLogHandler(QObject *parent)
    : QObject(parent)
{
    m_flushEachLog = qEnvironmentVariableIsEmpty(ENV_NO_FLUSH);
    m_verboseOutput = !qEnvironmentVariableIsEmpty(ENV_VERBOSE_OUTPUT);

#ifdef Q_OS_WIN
    m_isDebuggerPresent = IsDebuggerPresent();
#endif
}

ConsoleLogHandler::~ConsoleLogHandler() {}

void ConsoleLogHandler::handleLog(const Log &log)
{
    QString message;
    if (m_verboseOutput) {
        message = "[" + log.timestampUtc + "] " + log.category
                  + " | " + log.loglevelString + " | "
                  + log.locationString + "> " + log.message + "\n";
    } else {
        message = log.message + "\n";
    }

    std::ostream &stream = (log.loglevel == QtCriticalMsg || log.loglevel == QtFatalMsg) ? std::cerr : std::cout;
    stream << message.toStdString();

    if (m_flushEachLog)
        stream.flush();

#ifdef Q_OS_WIN
    // On windows, std::cout/std::cerr are captured by the debugger and e.g. in QtCreator not visible in the Application Output tab.
    // Qt already works around this in their qDebug implementation by calling OutputDebugStringW, so do the same if on windows and a debugger is detected.
    if (m_isDebuggerPresent) {
        OutputDebugStringA(message.toStdString().c_str());
    }
#endif
}

void ConsoleLogHandler::flush()
{
    std::cout.flush();
    std::cerr.flush();
}
