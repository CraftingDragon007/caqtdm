#ifndef TST_GENERALLOGHANDLER_H
#define TST_GENERALLOGHANDLER_H

#include <QObject>

class TestGeneralLogHandler : public QObject
{
    Q_OBJECT
public:
    TestGeneralLogHandler() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void injectsMessageHandlerAndReturnsPrevious();
    void initializationIsIdempotent();
    void callsHandlerWithMinLogLevel();
    void fatalMessageFlushesHandler();
    void logLevelIsLoadedFromEnv();
    void logHandlersAreInitializedFromEnv();
};

#endif // TST_GENERALLOGHANDLER_H
