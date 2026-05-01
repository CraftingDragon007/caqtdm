#include <QCoreApplication>
#include <QTest>

#include "tst_consoleloghandler.h"
#include "tst_fileloghandler.h"
#include "tst_generalloghandler.h"
#include "tst_logstashloghandler.h"

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Paul Scherrer Institut");
    QCoreApplication::setApplicationName("caQtDM-UnitTests-Logging");
    int status = 0;

    {
        TestGeneralLogHandler tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestConsoleLogHandler tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestFileLogHandler tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestLogstashLogHandler tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
