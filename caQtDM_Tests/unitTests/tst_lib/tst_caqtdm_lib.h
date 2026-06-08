#ifndef TST_CAQTDM_LIB_H
#define TST_CAQTDM_LIB_H

#include "fakefileopenwindow.h"

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
    void checkJsonStringWorks();
    void parseForDisplayRateWorks();
    void parseForQRectConstWorks();
    void treatMacroWorks();

private:
    FakeFileOpenWindow *m_fakeFileOpenWindow;
    MutexKnobData *m_mutexKnobData;
    CaQtDM_Lib *m_caQtDM_Lib;
};

#endif // TST_CAQTDM_LIB_H
