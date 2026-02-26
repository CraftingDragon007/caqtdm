#ifndef ABSTRACTLOGHANDLER_H
#define ABSTRACTLOGHANDLER_H

#include <QString>
#include <QtLogging>

typedef struct {
    const long long msSinceEpoch;
    const QString timestampUtc;
    const QtMsgType loglevel;
    const QString loglevelString;
    const QString message;
    const QString file;
    const QString function;
    const int line;
    const qint64 processId;
} Log;

class AbstractLogHandler
{
public:
    virtual ~AbstractLogHandler() = default;
    virtual void handleLog(const Log &log) = 0;
};

#endif // ABSTRACTLOGHANDLER_H
