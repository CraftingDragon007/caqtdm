#include "tst_pvdialog.h"

#include <QSignalSpy>
#include <QTest>
#include <pvdialog.h>

#include "abstractformwindowmanager.h"


void TestPVDialog::initTestCase()
{
    // code to be executed before the first test function

}

void TestPVDialog::init()
{
    // code to be executed before each test function

}

void TestPVDialog::cleanupTestCase()
{
    // code to be executed after the last test function


}

void TestPVDialog::cleanup()
{
    // code to be executed after each test function


}

void TestPVDialog::test1() {
    PVDialog dialog(Q_NULLPTR, Q_NULLPTR);
    qInfo() << dialog.sizeHint();
    qInfo() << (long long)dialog.entry;
    dialog.print_out(L"test");
    dialog.saveState();
}

void TestPVDialog::test2() {

}
