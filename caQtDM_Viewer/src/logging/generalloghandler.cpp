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

#define ENV_LOG_LEVEL "CAQTDM_LOGGING_LEVEL"
#define ENV_LOG_HANDLERS "CAQTDM_LOGGING_HANDLERS"

QMutex GeneralLogHandler::s_mutex;
QList<AbstractLogHandler *> GeneralLogHandler::s_logHandlers;
QThread *GeneralLogHandler::s_logHandlersThread = Q_NULLPTR;
QtMsgType GeneralLogHandler::s_minLogLevel = QtDebugMsg;

Q_LOGGING_CATEGORY(generalLogHandler, "logging.general");

QtMessageHandler GeneralLogHandler::initialize()
{
    QMutexLocker locker(&s_mutex);

    QtMessageHandler previousHandler = qInstallMessageHandler(GeneralLogHandler::messageHandler);
    if (previousHandler == GeneralLogHandler::messageHandler) {
        // Was already initialized
        return previousHandler;
    }

    // Re-install previous handler for initialization logs
    qInstallMessageHandler(previousHandler);

    // Clean up any previous handlers
    for (auto existingLogHandler : s_logHandlers) {
        delete existingLogHandler;
    }
    s_logHandlers.clear();

    s_minLogLevel = logLevelFromEnv();

    if (!s_logHandlersThread) {
        s_logHandlersThread = new QThread();
        s_logHandlersThread->setObjectName("LogHandlersThread");
        s_logHandlersThread->start();
    }

    const QStringList selectedLogHandlers = selectedLogHandlersFromEnv();
    if (selectedLogHandlers.contains("console")) {
        qCInfo(generalLogHandler) << "adding console log handler";
        ConsoleLogHandler *consoleLogHandler = new ConsoleLogHandler();
        consoleLogHandler->moveToThread(s_logHandlersThread);
        s_logHandlers.append(consoleLogHandler);
        QObject::connect(QCoreApplication::instance(),
                         &QCoreApplication::aboutToQuit,
                         consoleLogHandler,
                         &ConsoleLogHandler::flush,
                         Qt::QueuedConnection);
    }

    if (selectedLogHandlers.contains("file")) {
        qCInfo(generalLogHandler) << "adding file log handler";
        FileLogHandler *fileLogHandler = new FileLogHandler();
        fileLogHandler->moveToThread(s_logHandlersThread);
        s_logHandlers.append(fileLogHandler);
        QObject::connect(QCoreApplication::instance(),
                         &QCoreApplication::aboutToQuit,
                         fileLogHandler,
                         &FileLogHandler::flush,
                         Qt::QueuedConnection);
    }

    if (selectedLogHandlers.contains("logstash")) {
        qCInfo(generalLogHandler) << "adding logstash log handler";
        LogstashLogHandler *logstashLogHandler = new LogstashLogHandler();
        logstashLogHandler->moveToThread(s_logHandlersThread);
        s_logHandlers.append(logstashLogHandler);
        QObject::connect(QCoreApplication::instance(),
                         &QCoreApplication::aboutToQuit,
                         logstashLogHandler,
                         &LogstashLogHandler::flush,
                         Qt::QueuedConnection);
    }

#ifdef Q_OS_UNIX
    if (selectedLogHandlers.contains("syslog")) {
        qCInfo(generalLogHandler) << "adding syslog log handler";
        SyslogLogHandler *syslogLogHandler = new SyslogLogHandler();
        // Not a QObject, also no async operations, so not moved to separate thread.
        s_logHandlers.append(syslogLogHandler);
    }
#endif

    // Now the custom handler is ready to accept logs, so install it again
    qInstallMessageHandler(GeneralLogHandler::messageHandler);

    return previousHandler;
}

QtMsgType GeneralLogHandler::logLevelFromEnv(QtMsgType defaultLogLevel)
{
    const QString logLevelString = qgetenv(ENV_LOG_LEVEL).toLower();
    if (logLevelString.isEmpty()) {
        return defaultLogLevel;
    }

    if (logLevelString == "all" || logLevelString == "debug" || logLevelString == "qtdebugmsg") {
        return QtDebugMsg;
    } else if (logLevelString == "info" || logLevelString == "qtinfomsg") {
        return QtInfoMsg;
    } else if (logLevelString == "warning" || logLevelString == "qtwarningmsg") {
        return QtWarningMsg;
    } else if (logLevelString == "critical" || logLevelString == "qtcriticalmsg") {
        return QtCriticalMsg;
    } else if (logLevelString == "fatal" || logLevelString == "qtfatalmsg") {
        return QtFatalMsg;
    } else {
        qCWarning(generalLogHandler)
            << ENV_LOG_LEVEL
            << "is set and has a value, but could not be parsed. Using default log level:"
            << defaultLogLevel;
        return defaultLogLevel;
    }
}

QStringList GeneralLogHandler::selectedLogHandlersFromEnv(QString defaultConfig)
{
    if (!qEnvironmentVariableIsSet(ENV_LOG_HANDLERS)) {
        return QStringList(defaultConfig);
    }

    QStringList selectedLogHandlers;
    const QString config = qgetenv(ENV_LOG_HANDLERS).toLower().replace(" ", "");
    for (const auto &handler : config.split(',')) {
        if (handler == "console" || handler == "consoleloghandler") {
            selectedLogHandlers.append("console");
        } else if (handler == "file" || handler == "fileloghandler") {
            selectedLogHandlers.append("file");
        } else if (handler == "logstash" || handler == "logstashloghandler") {
            selectedLogHandlers.append("logstash");
        } else if (handler == "syslog" || handler == "syslogloghandler") {
#ifndef Q_OS_UNIX
            qCCritical(generalLogHandler)
                << ENV_LOG_HANDLERS
                << "specified syslog log handler, but this is invalid as system is not unix";
#else
            selectedLogHandlers.append("syslog");
#endif
        }
    }

    return selectedLogHandlers;
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
