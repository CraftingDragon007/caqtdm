#ifndef ABSTRACTLOGHANDLER_H
#define ABSTRACTLOGHANDLER_H

#include <QString>
#include <QtGlobal>

// Use this when implementing log handlers for known constant QStrings, it will result in compile-time optimizations
#define QSL(x) QStringLiteral(x)

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
    QString category;
    qint64 processId;
} Log;

class AbstractLogHandler
{
public:
    virtual ~AbstractLogHandler() = default;
    /**
     * @brief Can be called with logs to process.
     * This function may not be thread-safe.
     * @param log: The log, fields may be default-constructed
     */
    virtual void handleLog(const Log &log) = 0;
    /**
     * @brief Flushes all currently buffered logs (if any) such that they are processed finally.
     * This function may not be thread-safe. It must be blocking until all logs are flushed.
     */
    virtual void flush() = 0;

    /**
     * @brief Maps a numerical severity to a QtMsgType to compare against others.
     * @param type: The QtMsgType to map
     * @return The mapped severity
     */
    static constexpr int severity(const QtMsgType type)
    {
        return (type == QtDebugMsg)      ? 0
               : (type == QtInfoMsg)     ? 1
               : (type == QtWarningMsg)  ? 2
               : (type == QtCriticalMsg) ? 3
               : (type == QtFatalMsg)    ? 4
                                         : 3;
        // Not specified enum values values (last line, return = 3) are unspecified
        // (or deprecated QtSystemMsg which is = QtCriticalMsg either way),
        // so assume something went wrong badly and treat the same as QtCriticalMsg
    };
};

#endif // ABSTRACTLOGHANDLER_H
