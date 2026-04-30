#ifndef TST_LOGSTASHLOGHANDLER_H
#define TST_LOGSTASHLOGHANDLER_H

#include <QObject>

class TestLogstashLogHandler : public QObject
{
    Q_OBJECT
public:
    TestLogstashLogHandler() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void parametersInitializedFromEnv();
    void flushClearsBuffer();
    void bufferMaxSizeFlushes();
    void bufferTimeoutFlushes();
    void flushSendsPayload();
};

#endif // TST_LOGSTASHLOGHANDLER_H
