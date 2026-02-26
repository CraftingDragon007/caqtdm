#include "generalloghandler.h"

#include "consoleloghandler.h"

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

    Log log = {QDateTime::currentDateTime(),
               type,
               message,
               context.file,
               context.function,
               context.line,
               QCoreApplication::applicationPid()};

    for (auto logHandler : s_logHandlers) {
        logHandler->handleLog(log);
    }
}
