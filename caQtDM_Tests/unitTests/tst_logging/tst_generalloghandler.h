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
    void test_case1();
};

#endif // TST_GENERALLOGHANDLER_H
