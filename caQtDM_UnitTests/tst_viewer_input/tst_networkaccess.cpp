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

#include "tst_networkaccess.h"

#include <QDir>
#include <QFile>
#include <QTest>

#include "fakenetworkaccessmanager.h"
#include "networkaccess.h"
#include "specialFunctions.h"

static const char *PAYLOAD = "<?xml version=\"1.0\"?><ui version=\"4.0\"/>";

// the download directory is shared with a possibly running caQtDM, so every
// name a test creates or removes carries the prefix tst_vi_

void TestNetworkAccess::initTestCase()
{
    Specials specials;
    m_stdPath = specials.getStdPath();
    QVERIFY2(!m_stdPath.isEmpty(), "download directory must be known");
    QDir().mkpath(m_stdPath);
}

void TestNetworkAccess::cleanup()
{
    // remove what a test may have created, so the next one starts clean
    QFile::remove(m_stdPath + "/tst_vi_panel.ui");
    QDir(m_stdPath + "/tst_vi_sub").removeRecursively();
    QFile::remove(m_stdPath + "/tst_vi_my..file.ui");
    QDir(m_stdPath + "/tst_vi_a").removeRecursively();
    QFile::remove(m_stdPath + "/tst_vi_evil.ui");
    // a run with a broken guard would drop this one above the directory
    QFile::remove(QDir(m_stdPath).filePath("../tst_vi_evil.ui"));
}

void TestNetworkAccess::downloadsPlainFile()
{
    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    NetworkAccess access(&manager);

    const bool ok = access.requestUrl(QUrl("http://example.invalid/panel.ui"), "tst_vi_panel.ui");

    QVERIFY2(ok, "the mocked download has to report success");
    QCOMPARE(manager.requestCount(), 1);

    QFile written(m_stdPath + "/tst_vi_panel.ui");
    QVERIFY2(written.exists(), "file has to be stored in the download directory");
    QVERIFY(written.open(QIODevice::ReadOnly));
    QCOMPARE(written.readAll(), QByteArray(PAYLOAD));
    written.close();
}

void TestNetworkAccess::downloadsIntoSubdirectory()
{
    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    NetworkAccess access(&manager);

    const bool ok = access.requestUrl(QUrl("http://example.invalid/sub/panel.ui"),
                                      "tst_vi_sub/tst_vi_panel.ui");

    QVERIFY2(ok, "a relative subdirectory is a legal target");
    QVERIFY(QFile::exists(m_stdPath + "/tst_vi_sub/tst_vi_panel.ui"));
}

void TestNetworkAccess::downloadsWithoutTargetFile()
{
    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    NetworkAccess access(&manager);

    // without a file name the reply is fetched but nothing is written
    const bool ok = access.requestUrl(QUrl("http://example.invalid/ping"), QString());

    QVERIFY2(ok, "a download without target file has to report success");
    QCOMPARE(manager.requestCount(), 1);
}

void TestNetworkAccess::reportsHttpError()
{
    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    manager.failWithHttpStatus(404, "Not Found");
    NetworkAccess access(&manager);

    const bool ok = access.requestUrl(QUrl("http://example.invalid/panel.ui"), "tst_vi_panel.ui");

    QVERIFY2(!ok, "an http error must not report success");
    QVERIFY2(access.lastError().contains("404"),
             qPrintable(QString("status code has to be reported, got: %1").arg(access.lastError())));
    QVERIFY2(!QFile::exists(m_stdPath + "/tst_vi_panel.ui"),
             "nothing may be written for a failed download");
}

void TestNetworkAccess::timesOutWhenReplyNeverFinishes()
{
    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    manager.neverFinish();
    NetworkAccess access(&manager);

    // runs into the 3 second timeout of NetworkAccess
    const bool ok = access.requestUrl(QUrl("http://example.invalid/panel.ui"), "tst_vi_panel.ui");

    QVERIFY2(!ok, "a request that never finishes must not report success");
    QVERIFY2(access.lastError().contains("timeout"),
             qPrintable(QString("timeout has to be reported, got: %1").arg(access.lastError())));
}

void TestNetworkAccess::rejectsTraversal_data()
{
    QTest::addColumn<QString>("fileName");
    // path that must not appear, empty when there is no single obvious target
    QTest::addColumn<QString>("mustNotExist");

    QTest::newRow("parent") << "../tst_vi_evil.ui" << "tst_vi_evil.ui";
    QTest::newRow("parent twice") << "tst_vi_a/../../tst_vi_evil.ui" << "tst_vi_evil.ui";
    QTest::newRow("bare dotdot") << ".." << QString();
    QTest::newRow("deep") << "tst_vi_sub/../../../tst_vi_evil.ui" << QString();

    // a backslash is a separator on windows only; elsewhere it is an ordinary
    // character in a file name and therefore no traversal at all
#ifdef Q_OS_WIN
    QTest::newRow("backslash") << "..\\tst_vi_evil.ui" << "tst_vi_evil.ui";
#endif
}

void TestNetworkAccess::rejectsTraversal()
{
    QFETCH(QString, fileName);
    QFETCH(QString, mustNotExist);

    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    NetworkAccess access(&manager);

    const bool ok = access.requestUrl(QUrl("http://example.invalid/x"), fileName);

    QVERIFY2(!ok, qPrintable(QString("%1 must not report success").arg(fileName)));
    QVERIFY2(access.lastError().contains("refused"),
             qPrintable(QString("rejection has to be reported for %1, got: %2")
                            .arg(fileName, access.lastError())));

    if (!mustNotExist.isEmpty()) {
        // the sibling of the download directory is where a traversal would land
        const QString escaped = QDir(m_stdPath).filePath("../" + mustNotExist);
        QVERIFY2(!QFile::exists(escaped),
                 qPrintable(QString("nothing may be written outside for %1").arg(fileName)));
    }
}

void TestNetworkAccess::acceptsDotsInsideName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("resolved");

    // ".." only counts as a whole path segment, not as a substring
    QTest::newRow("dots in filename") << "tst_vi_my..file.ui" << "tst_vi_my..file.ui";
    QTest::newRow("dots starting a directory") << "tst_vi_a/..b/c.ui" << "tst_vi_a/..b/c.ui";
    // a ".." that stays below the download directory is harmless
    QTest::newRow("dotdot cancelled out") << "tst_vi_sub/../tst_vi_panel.ui" << "tst_vi_panel.ui";
    QTest::newRow("dotdot deeper") << "tst_vi_a/b/../c.ui" << "tst_vi_a/c.ui";
    // an absolute path is taken relative to the download directory, not the root
    QTest::newRow("absolute path") << "/tst_vi_evil.ui" << "tst_vi_evil.ui";
}

void TestNetworkAccess::acceptsDotsInsideName()
{
    QFETCH(QString, fileName);
    QFETCH(QString, resolved);

    FakeNetworkAccessManager manager((QByteArray(PAYLOAD)));
    NetworkAccess access(&manager);

    const bool ok = access.requestUrl(QUrl("http://example.invalid/x"), fileName);

    QVERIFY2(ok, qPrintable(QString("%1 must not be treated as traversal").arg(fileName)));
    QVERIFY2(QFile::exists(m_stdPath + "/" + resolved),
             qPrintable(QString("%1 has to end up as %2").arg(fileName, resolved)));
}
