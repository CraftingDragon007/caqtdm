#ifndef CONSOLELOGHANDLER_H
#define CONSOLELOGHANDLER_H

#include "abstractloghandler.h"

#include <QObject>

class ConsoleLogHandler : public QObject, public AbstractLogHandler
{
    Q_OBJECT
public:
    explicit ConsoleLogHandler(QObject *parent = Q_NULLPTR);
    ~ConsoleLogHandler() override;

    /**
     * @brief Prints the passed log to stdout or sterr, based on QtMsgType.
     * This function is not thread-safe.
     * @param log: The log to print
     */
    void handleLog(const Log &log) override;

public slots:
    /**
     * @brief Flushes stdout and stderr.
     * This function is not guaranteed to be thread-safe.
     */
    void flush() override;

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    bool m_flushEachLog;
    bool m_verboseOutput;

#ifdef Q_OS_WIN
    bool m_isDebuggerPresent;
#endif
};

#endif // CONSOLELOGHANDLER_H
