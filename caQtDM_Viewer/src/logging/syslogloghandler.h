#ifndef SYSLOGLOGHANDLER_H
#define SYSLOGLOGHANDLER_H

#include "abstractloghandler.h"

class SyslogLogHandler : public AbstractLogHandler
{
public:
    SyslogLogHandler();
    ~SyslogLogHandler() override;

    /**
     * @brief Calls syslog with log.
     * This function is thread-safe.
     * @param log: The log to process.
     */
    void handleLog(const Log &log) override;
    /**
     * @brief Does nothing.
     */
    void flush() override {};
};

#endif // SYSLOGLOGHANDLER_H
