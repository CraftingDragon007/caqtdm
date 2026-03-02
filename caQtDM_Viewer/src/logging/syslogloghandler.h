#ifndef SYSLOGLOGHANDLER_H
#define SYSLOGLOGHANDLER_H

#include "abstractloghandler.h"

class SyslogLogHandler : public AbstractLogHandler
{
public:
    SyslogLogHandler();
    ~SyslogLogHandler() override;

    void handleLog(const Log &log) override;
    void flush() override {};
};

#endif // SYSLOGLOGHANDLER_H
