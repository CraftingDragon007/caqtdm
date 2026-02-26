#include "generalloghandler.h"

#include "consoleloghandler.h"

#include <chrono>
#include <ctime>
#include <cstdio>

#include <QCoreApplication>

QMutex GeneralLogHandler::s_mutex;
QList<AbstractLogHandler*> GeneralLogHandler::s_logHandlers;

QtMessageHandler GeneralLogHandler::initialize()
{
    QMutexLocker locker(&s_mutex);

    QtMessageHandler previousHandler = qInstallMessageHandler(GeneralLogHandler::messageHandler);
    if (previousHandler == GeneralLogHandler::messageHandler) {
        // Was already initialized
        return previousHandler;
    }

    // Clean up any previous handlers
    for (auto existingLogHandler : s_logHandlers) {
        delete existingLogHandler;
    }
    s_logHandlers.clear();

    // Add console handler
    ConsoleLogHandler *consoleLogHandler = new ConsoleLogHandler();
    s_logHandlers.append(consoleLogHandler);

    return previousHandler;
}

void GeneralLogHandler::messageHandler(QtMsgType type,
                                       const QMessageLogContext &context,
                                       const QString &message)
{
    QMutexLocker locker(&s_mutex);

    QString logLevelString;
    switch (type) {
    case QtDebugMsg:
        logLevelString = "QtDebugMsg";
        break;
    case QtInfoMsg:
        logLevelString = "QtInfoMsg";
        break;
    case QtWarningMsg:
        logLevelString = "QtWarningMsg";
        break;
    case QtCriticalMsg:
        logLevelString = "QtCriticalMsg";
        break;
    case QtFatalMsg:
        logLevelString = "QtFatalMsg";
        break;
    default:
        logLevelString = "unkown QtMsgType";
    }

    long long msSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();

    std::time_t seconds = msSinceEpoch / 1000;
    int milliseconds = msSinceEpoch % 1000;
    std::tm tm = *std::gmtime(&seconds);
    char timestampUtc[32];
    std::snprintf(timestampUtc, sizeof(timestampUtc),
                  "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                  tm.tm_year + 1900,
                  tm.tm_mon + 1,
                  tm.tm_mday,
                  tm.tm_hour,
                  tm.tm_min,
                  tm.tm_sec,
                  milliseconds);

    Log log = {msSinceEpoch,
               timestampUtc,
               type,
               logLevelString,
               message,
               context.file,
               context.function,
               context.line,
               QCoreApplication::applicationPid()};

    for (auto logHandler : s_logHandlers) {
        logHandler->handleLog(log);
    }
}
