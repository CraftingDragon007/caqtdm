#include <QCoreApplication>
#include <QTest>

#include "tst_caqtdm_lib.h"

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    qputenv("QT_QPA_FONTDIR", QByteArrayLiteral("."));

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Paul Scherrer Institut");
    QApplication::setApplicationName("caQtDM-UnitTests-Lib");
    int status = 0;

    {
        TestCaQtDM_Lib tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
