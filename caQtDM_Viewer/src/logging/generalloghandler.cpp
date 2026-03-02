#include "generalloghandler.h"

#include "consoleloghandler.h"
#include "fileloghandler.h"
#include "logstashloghandler.h"
#ifdef Q_OS_UNIX
#include "syslogloghandler.h"
#endif

#include <chrono>
#include <cstdio>
#include <ctime>

#include <QCoreApplication>

QMutex GeneralLogHandler::s_mutex;
QList<AbstractLogHandler *> GeneralLogHandler::s_logHandlers;
QThread *GeneralLogHandler::s_logHandlersThread = Q_NULLPTR;
QtMsgType GeneralLogHandler::s_minLogLevel = QtDebugMsg;

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

    if (!s_logHandlersThread) {
        s_logHandlersThread = new QThread();
        s_logHandlersThread->setObjectName("LogHandlersThread");
        s_logHandlersThread->start();
    }

    ConsoleLogHandler *consoleLogHandler = new ConsoleLogHandler();
    consoleLogHandler->moveToThread(s_logHandlersThread);
    s_logHandlers.append(consoleLogHandler);
    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     consoleLogHandler,
                     &ConsoleLogHandler::flush,
                     Qt::QueuedConnection);

    FileLogHandler *fileLogHandler = new FileLogHandler();
    fileLogHandler->moveToThread(s_logHandlersThread);
    s_logHandlers.append(fileLogHandler);
    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     fileLogHandler,
                     &FileLogHandler::flush,
                     Qt::QueuedConnection);

    LogstashLogHandler *logstashLogHandler = new LogstashLogHandler();
    logstashLogHandler->moveToThread(s_logHandlersThread);
    s_logHandlers.append(logstashLogHandler);
    QObject::connect(QCoreApplication::instance(),
                     &QCoreApplication::aboutToQuit,
                     logstashLogHandler,
                     &LogstashLogHandler::flush,
                     Qt::QueuedConnection);

#ifdef Q_OS_UNIX
    SyslogLogHandler *syslogLogHandler = new SyslogLogHandler();
    // Not a QObject, also no async operations, so not moved to separate thread.
    s_logHandlers.append(syslogLogHandler);
#endif

    return previousHandler;
}

void GeneralLogHandler::messageHandler(QtMsgType type,
                                       const QMessageLogContext &context,
                                       const QString &message)
{
    if (AbstractLogHandler::severity(type) < AbstractLogHandler::severity(s_minLogLevel)) {
        return;
    }

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
    std::tm tm;
#if defined(_WIN32)
    gmtime_s(&tm, &seconds);
#else
    gmtime_r(&seconds, &tm);
#endif
    char timestampUtc[32];
    std::snprintf(timestampUtc,
                  sizeof(timestampUtc),
                  "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                  tm.tm_year + 1900,
                  tm.tm_mon + 1,
                  tm.tm_mday,
                  tm.tm_hour,
                  tm.tm_min,
                  tm.tm_sec,
                  milliseconds);

    const QString locationString = QString(context.file) + ":" + context.function + ":"
                                   + QString::number(context.line);

    QString truncatedMessage = message;
    // No need for trailing newlines, log handlers should receive message without them.
    if (message.endsWith('\n')) {
        truncatedMessage.remove(truncatedMessage.size() - 1, 1);
    }

    Log log = {msSinceEpoch,
               timestampUtc,
               type,
               logLevelString,
               truncatedMessage,
               locationString,
               context.file,
               context.function,
               context.line,
               context.category,
               QCoreApplication::applicationPid()};

    QMutexLocker locker(&s_mutex);
    for (auto logHandler : s_logHandlers) {
        logHandler->handleLog(log);
    }

    // Qt will exit faulty after this returns, so make sure to flush
    if (type == QtFatalMsg) {
        for (auto logHandler : s_logHandlers) {
            logHandler->flush();
        }
    }
}
