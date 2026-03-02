#ifndef ABSTRACTLOGHANDLER_H
#define ABSTRACTLOGHANDLER_H

#include <QString>
#include <qlogging.h>

typedef struct
{
    long long msSinceEpoch;
    QString timestampUtc;
    QtMsgType loglevel;
    QString loglevelString;
    QString message;
    QString locationString;
    QString file;
    QString function;
    int line;
    qint64 processId;
} Log;

class AbstractLogHandler
{
public:
    virtual ~AbstractLogHandler() = default;
    virtual void handleLog(const Log &log) = 0;
    virtual void flush() = 0;
};

#endif // ABSTRACTLOGHANDLER_H
