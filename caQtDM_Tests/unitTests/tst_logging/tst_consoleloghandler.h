#ifndef TST_CONSOLELOGHANDLER_H
#define TST_CONSOLELOGHANDLER_H

#include <QObject>

class TestConsoleLogHandler : public QObject
{
    Q_OBJECT
public:
    TestConsoleLogHandler() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void test_case1();
};

#endif // TST_CONSOLELOGHANDLER_H
