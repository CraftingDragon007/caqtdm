#ifndef TST_CAQTDM_LIB_H
#define TST_CAQTDM_LIB_H

#include <caqtdm_lib.h>

#include <QObject>
#include <QTest>

class TestCaQtDM_Lib : public QObject
{
    Q_OBJECT
public:
    TestCaQtDM_Lib() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void test1();
private:
};

#endif // TST_CAQTDM_LIB_H
