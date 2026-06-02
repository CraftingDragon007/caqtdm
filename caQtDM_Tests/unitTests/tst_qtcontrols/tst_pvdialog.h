#ifndef TST_PVDIALOG_H
#define TST_PVDIALOG_H

#include <QObject>

class TestPVDialog : public QObject
{
    Q_OBJECT
public:
    TestPVDialog() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void test1();
    void test2();
};

#endif // TST_PVDIALOG_H
