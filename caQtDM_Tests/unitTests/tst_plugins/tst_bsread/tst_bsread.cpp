#include <QCoreApplication>
#include <QTest>

#include "tst_bsread_decode.h"
#include "tst_bsread_dispatchercontrol.h"

Q_LOGGING_CATEGORY(bsreadLog, "caqtdm.plugins.bsread")

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    qputenv("QT_QPA_FONTDIR", QByteArrayLiteral("."));

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Paul Scherrer Institut");
    QApplication::setApplicationName("caQtDM-UnitTests-bsread");
    int status = 0;

    {
        Testbsread_dispatchercontrol tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        Testbsread_Decode tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
