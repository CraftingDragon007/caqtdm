#ifndef ABSTRACTLOGHANDLER_H
#define ABSTRACTLOGHANDLER_H

#include <QString>
#include <QtGlobal>

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

    static constexpr int severity(QtMsgType type)
    {
        return (type == QtInfoMsg)       ? 0
               : (type == QtDebugMsg)    ? 1
               : (type == QtWarningMsg)  ? 2
               : (type == QtCriticalMsg) ? 3
               : (type == QtFatalMsg)    ? 4
                                         : 3;
        // Not specified enum values values (last line, return = 3) are unspecified (or deprecated QtSystemMsg which is = QtCriticalMsg either way),
        // so assume something went wrong badly and treat the same as QtCriticalMsg
    };
};

#endif // ABSTRACTLOGHANDLER_H
