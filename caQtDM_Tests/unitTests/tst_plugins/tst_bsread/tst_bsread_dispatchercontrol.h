#ifndef TST_BSREAD_DISPATCHERCONTROL_H
#define TST_BSREAD_DISPATCHERCONTROL_H

#include <bsread_dispatchercontrol.h>

#include <QObject>
#include <QTest>

class Testbsread_dispatchercontrol : public QObject
{
    Q_OBJECT
public:
    Testbsread_dispatchercontrol() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void test1();
    void test2();
    void test3();
    void test4();

private:
};

#endif // TST_BSREAD_DISPATCHERCONTROL_H
