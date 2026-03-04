#ifndef TST_FILELOGHANDLER_H
#define TST_FILELOGHANDLER_H

#include <QObject>

class TestFileLogHandler : public QObject
{
    Q_OBJECT
public:
    TestFileLogHandler() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void test_case1();
};

#endif // TST_FILELOGHANDLER_H
