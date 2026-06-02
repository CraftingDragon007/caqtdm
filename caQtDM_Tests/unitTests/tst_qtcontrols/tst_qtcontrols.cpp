#include <QCoreApplication>
#include <QTest>

#include "tst_pvdialog.h"

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Paul Scherrer Institut");
    QApplication::setApplicationName("caQtDM-UnitTests-QtControls");
    int status = 0;

    {
        TestPVDialog tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
