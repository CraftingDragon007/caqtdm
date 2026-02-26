#ifndef CONSOLELOGHANDLER_H
#define CONSOLELOGHANDLER_H

#include "abstractloghandler.h"

class ConsoleLogHandler : public AbstractLogHandler
{
public:
    ConsoleLogHandler();
    ~ConsoleLogHandler() override;

    void handleLog(const Log &log) override;
};

#endif // CONSOLELOGHANDLER_H
