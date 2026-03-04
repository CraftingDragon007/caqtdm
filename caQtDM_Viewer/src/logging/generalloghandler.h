#ifndef GENERALLOGHANDLER_H
#define GENERALLOGHANDLER_H

#include "abstractloghandler.h"

#include <QMutex>
#include <QThread>

#define DEFAULT_LOG_LEVEL QtDebugMsg

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(generalLogHandler);

class GeneralLogHandler
{
public:
    GeneralLogHandler() = delete;
    static QtMessageHandler initialize();
    static void messageHandler(QtMsgType type,
                               const QMessageLogContext &context,
                               const QString &message);

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    static QtMsgType logLevelFromEnv(QtMsgType defaultLogLevel = DEFAULT_LOG_LEVEL);

    static QMutex s_mutex;
    static QList<AbstractLogHandler *> s_logHandlers;
    static QThread *s_logHandlersThread;
    static QtMsgType s_minLogLevel;
};

#endif // GENERALLOGHANDLER_H
