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

#include "tst_envfilter.h"

#include <QDir>
#include <QFile>
#include <QTest>
#include <QTextStream>

#include "fileopenwindow.h"
#include "specialFunctions.h"

// the variables a downloaded configuration file is allowed to influence
static const char *REAL_CONFIG =
    "EPICS_CA_MAX_ARRAY_BYTES 150000000\n"
    "EPICS_CA_AUTO_ADDR_LIST NO\n"
    "EPICS_CA_ADDR_LIST proscan-cagw01.psi.ch\n"
    "EPICS_CA_SERVER_PORT 5062\n"
    "CAQTDM_LAUNCHFILE proscan_overview.ui\n"
    "CAQTDM_URL_DISPLAY_PATH https://example.org/panels\n";

void TestEnvFilter::initTestCase()
{
    Specials specials;
    m_stdPath = specials.getStdPath();
    QVERIFY2(!m_stdPath.isEmpty(), "download directory must be known");
    QDir().mkpath(m_stdPath);
    m_configName = "tst_envfilter.config";
}

void TestEnvFilter::init()
{
    m_savedEnv.clear();
}

void TestEnvFilter::cleanup()
{
    // restore the process environment so the tests stay independent
    for (QMap<QString, QByteArray>::const_iterator it = m_savedEnv.constBegin();
         it != m_savedEnv.constEnd(); ++it) {
        if (it.value().isNull()) qunsetenv(it.key().toLatin1().constData());
        else qputenv(it.key().toLatin1().constData(), it.value());
    }
    m_savedEnv.clear();
    QFile::remove(m_stdPath + "/" + m_configName);
}

void TestEnvFilter::rememberEnv(const QString &name)
{
    if (m_savedEnv.contains(name)) return;
    const QByteArray key = name.toLatin1();
    // a null QByteArray marks "was not set before"
    if (qEnvironmentVariableIsSet(key.constData())) m_savedEnv.insert(name, qgetenv(key.constData()));
    else m_savedEnv.insert(name, QByteArray());
}

// returns false when the config file could not be written; the caller wraps
// the call in QVERIFY so a failure aborts the test slot itself
bool TestEnvFilter::applyConfig(const QString &content)
{
    // the method always rewrites this one, so it has to be restored as well
    rememberEnv("CAQTDM_DISPLAY_PATH");

    QFile file(m_stdPath + "/" + m_configName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    QTextStream out(&file);
    out << content;
    file.close();

    // no MessageWindow in this test, the method has to cope with that
    FileOpenWindow::setAllEnvironmentVariables(m_configName, Q_NULLPTR);
    return true;
}

void TestEnvFilter::appliesRealWorldConfiguration()
{
    const QStringList names = QStringList() << "EPICS_CA_MAX_ARRAY_BYTES"
                                            << "EPICS_CA_AUTO_ADDR_LIST"
                                            << "EPICS_CA_ADDR_LIST"
                                            << "EPICS_CA_SERVER_PORT"
                                            << "CAQTDM_LAUNCHFILE"
                                            << "CAQTDM_URL_DISPLAY_PATH";
    foreach (const QString &name, names) {
        rememberEnv(name);
        qunsetenv(name.toLatin1().constData());
    }

    QVERIFY(applyConfig(QString::fromLatin1(REAL_CONFIG)));

    QCOMPARE(qgetenv("EPICS_CA_MAX_ARRAY_BYTES"), QByteArray("150000000"));
    QCOMPARE(qgetenv("EPICS_CA_AUTO_ADDR_LIST"), QByteArray("NO"));
    QCOMPARE(qgetenv("EPICS_CA_ADDR_LIST"), QByteArray("proscan-cagw01.psi.ch"));
    QCOMPARE(qgetenv("EPICS_CA_SERVER_PORT"), QByteArray("5062"));
    QCOMPARE(qgetenv("CAQTDM_LAUNCHFILE"), QByteArray("proscan_overview.ui"));
    QCOMPARE(qgetenv("CAQTDM_URL_DISPLAY_PATH"), QByteArray("https://example.org/panels"));

    // the method forces this one to the download directory, no matter what
    QCOMPARE(QString::fromLocal8Bit(qgetenv("CAQTDM_DISPLAY_PATH")), m_stdPath);
}

void TestEnvFilter::blocksDangerousNames_data()
{
    QTest::addColumn<QString>("name");

    QTest::newRow("qt plugins") << "QT_PLUGIN_PATH";
    QTest::newRow("qt platform plugins") << "QT_QPA_PLATFORM_PLUGIN_PATH";
    QTest::newRow("qml import path") << "QML_IMPORT_PATH";
    QTest::newRow("ld preload") << "LD_PRELOAD";
    QTest::newRow("ld library path") << "LD_LIBRARY_PATH";
    QTest::newRow("dyld insert") << "DYLD_INSERT_LIBRARIES";
    QTest::newRow("dyld library path") << "DYLD_LIBRARY_PATH";
    QTest::newRow("path") << "PATH";
    QTest::newRow("caqtdm exec list") << "CAQTDM_EXEC_LIST";
    QTest::newRow("medm exec list") << "MEDM_EXEC_LIST";
    QTest::newRow("caqtdm web path") << "CAQTDM_WEB_PATH";
    QTest::newRow("pythonpath") << "PYTHONPATH";
    QTest::newRow("pythonhome") << "PYTHONHOME";
    QTest::newRow("ifs") << "IFS";
    QTest::newRow("bash env") << "BASH_ENV";
    QTest::newRow("env") << "ENV";
}

void TestEnvFilter::blocksDangerousNames()
{
    QFETCH(QString, name);

    rememberEnv(name);
    qputenv(name.toLatin1().constData(), QByteArray("untouched"));

    QVERIFY(applyConfig(name + " /tmp/injected\n"));

    QCOMPARE(qgetenv(name.toLatin1().constData()), QByteArray("untouched"));
}

void TestEnvFilter::ignoresMalformedNames_data()
{
    QTest::addColumn<QString>("line");
    QTest::addColumn<QString>("name");

    QTest::newRow("lowercase") << "caqtdm_lower value" << "caqtdm_lower";
    QTest::newRow("dash") << "CAQTDM-DASH value" << "CAQTDM-DASH";
    QTest::newRow("too long")
        << QString("CAQTDM_%1 value").arg(QString(70, QLatin1Char('X')))
        << QString("CAQTDM_%1").arg(QString(70, QLatin1Char('X')));
}

void TestEnvFilter::ignoresMalformedNames()
{
    QFETCH(QString, line);
    QFETCH(QString, name);

    rememberEnv(name);
    qunsetenv(name.toLatin1().constData());

    QVERIFY(applyConfig(line + "\n"));

    QVERIFY2(!qEnvironmentVariableIsSet(name.toLatin1().constData()),
             qPrintable(QString("%1 must not be set").arg(name)));
}

void TestEnvFilter::ignoresOversizedValue()
{
    const QString name = "CAQTDM_HUGE_VALUE";
    rememberEnv(name);
    qunsetenv(name.toLatin1().constData());

    QVERIFY(applyConfig(name + " " + QString(5000, QLatin1Char('a')) + "\n"));

    QVERIFY2(!qEnvironmentVariableIsSet(name.toLatin1().constData()),
             "an oversized value must be rejected");
}

void TestEnvFilter::acceptsMaxLengthName()
{
    // 64 characters is the documented limit and still legal
    const QString name = QString("CAQTDM_%1").arg(QString(57, QLatin1Char('X')));
    QCOMPARE(name.size(), 64);
    rememberEnv(name);
    qunsetenv(name.toLatin1().constData());

    QVERIFY(applyConfig(name + " value\n"));

    QCOMPARE(qgetenv(name.toLatin1().constData()), QByteArray("value"));
}

void TestEnvFilter::acceptsMaxLengthValue()
{
    // 4096 characters is the documented limit and still legal
    const QString name = "CAQTDM_MAX_VALUE";
    const QString value = QString(4096, QLatin1Char('a'));
    rememberEnv(name);
    qunsetenv(name.toLatin1().constData());

    QVERIFY(applyConfig(name + " " + value + "\n"));

    QCOMPARE(qgetenv(name.toLatin1().constData()), value.toLatin1());
}

void TestEnvFilter::ignoresLineWithoutSpace()
{
    // "NAME=value" is one token; the file format demands a space
    const QString name = "CAQTDM_EQUALS_TEST";
    rememberEnv(name);
    qunsetenv(name.toLatin1().constData());

    QVERIFY(applyConfig(name + "=value\n"));

    QVERIFY2(!qEnvironmentVariableIsSet(name.toLatin1().constData()),
             "a line without a space separator must not set anything");
}
