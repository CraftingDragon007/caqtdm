/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2026
 */

// Tests for everything the viewer accepts from outside: files fetched over http
// and the environment taken from a downloaded configuration file.

#include <QApplication>
#include <QLoggingCategory>
#include <QTest>

#include "loggingcategories.h"
#include "tst_envfilter.h"
#include "tst_networkaccess.h"

#ifdef WEB
// fileopenwindow.cpp uses this category in its WEB branches; the application
// defines it in caQtDM.cpp, which cannot be part of a test binary
Q_LOGGING_CATEGORY(webLog, "caqtdm.viewer.web")
#endif

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    qputenv("QT_QPA_FONTDIR", QByteArrayLiteral("."));

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Paul Scherrer Institut");
    QCoreApplication::setApplicationName("caQtDM-UnitTests-Viewer-Input");
    setlocale(LC_NUMERIC, "C");
    int status = 0;

    {
        TestNetworkAccess tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestEnvFilter tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
