#include "tst_caqtdm_lib.h"

#include "fakefileopenwindow.h"

void TestCaQtDM_Lib::initTestCase()
{
    // code to be executed before the first test function
}

void TestCaQtDM_Lib::init()
{
    // code to be executed before each test function
}

void TestCaQtDM_Lib::cleanupTestCase()
{
    // code to be executed after the last test function
}

void TestCaQtDM_Lib::cleanup()
{
    // code to be executed after each test function
}

void TestCaQtDM_Lib::test1() {
    caLed parentAS(Q_NULLPTR);
    FakeFileOpenWindow parent(Q_NULLPTR);
    MutexKnobData mutexKnobData;
    CaQtDM_Lib lib(&parent, "", "", &mutexKnobData, {}, Q_NULLPTR, false, &parentAS, {});
}
