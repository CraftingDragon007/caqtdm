#ifndef GENERALLOGHANDLER_H
#define GENERALLOGHANDLER_H

#include "abstractloghandler.h"

#include <QMutex>

class GeneralLogHandler
{
public:
    GeneralLogHandler() = delete;
    static QtMessageHandler initialize();
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);
private:
    static QMutex s_mutex;
    static QList<AbstractLogHandler*> s_logHandlers;
};

#endif // GENERALLOGHANDLER_H
