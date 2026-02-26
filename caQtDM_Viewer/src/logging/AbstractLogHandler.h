#ifndef ABSTRACTLOGHANDLER_H
#define ABSTRACTLOGHANDLER_H

#include <QDateTime>
#include <QtLogging>

typedef struct {
    const QDateTime timestamp;
    const QtMsgType logLevel;
    const QString message;
    const QString fileName;
    const QString functionName;
    const int lineNumber;
    const qint64 processId;
} Log;

class AbstractLogHandler
{
public:
    virtual ~AbstractLogHandler() = default;
    virtual void handleLog(const Log &log) = 0;
};

#endif // ABSTRACTLOGHANDLER_H
