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

    void handleLog(const Log &log) override;

public slots:
    void flush() override;

private:
    bool m_flushEachLog;
    bool m_verboseOutput;
};

#endif // CONSOLELOGHANDLER_H
