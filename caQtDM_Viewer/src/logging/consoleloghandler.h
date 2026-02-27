#ifndef CONSOLELOGHANDLER_H
#define CONSOLELOGHANDLER_H

#include "abstractloghandler.h"

class ConsoleLogHandler : public AbstractLogHandler
{
public:
    ConsoleLogHandler();
    ~ConsoleLogHandler() override;

    void handleLog(const Log &log) override;

private:
    bool m_flushEachLog;
    bool m_verboseOutput;
};

#endif // CONSOLELOGHANDLER_H
