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
 *
 *  Author:
 *    Helge Brands
 */

#include "tst_caspinbox.h"

#include <QApplication>
#include <clocale>

void TestCaSpinbox::initTestCase()
{
    // to avoid failing tests because of a comma as decimal separator
    setlocale(LC_NUMERIC, "C");
}

void TestCaSpinbox::incrementDecrementByButtons_data()
{
    QTest::addColumn<int>("intDig");
    QTest::addColumn<int>("decDig");
    QTest::addColumn<int>("index");
    QTest::addColumn<bool>("up");
    QTest::addColumn<double>("startValue");
    QTest::addColumn<long long>("expectedDeltaData");
    QTest::addColumn<int>("expectSignals"); // -1: do not check

    QTest::newRow("LSD up") << 3 << 2 << 4 << true << 0.0 << Q_INT64_C(1) << 1;
    QTest::newRow("LSD down") << 3 << 2 << 4 << false << 0.0 << Q_INT64_C(-1) << 1;
    QTest::newRow("MSD up") << 3 << 2 << 0 << true << 0.0 << Q_INT64_C(10000) << 1;
    QTest::newRow("MSD down") << 3 << 2 << 0 << false << 0.0 << Q_INT64_C(-10000) << 1;
    QTest::newRow("down below minimum is ignored")
        << 1 << 0 << 0 << false << -9.0 << Q_INT64_C(0) << 0;
    /* data beyond 2^53: exact only with pure integer increments */
    QTest::newRow("LSD up beyond 2^53") << 17 << 1 << 17 << true << 9.0e15 << Q_INT64_C(1) << -1;
}

void TestCaSpinbox::incrementDecrementByButtons()
{
    QFETCH(int, intDig);
    QFETCH(int, decDig);
    QFETCH(int, index);
    QFETCH(bool, up);
    QFETCH(double, startValue);
    QFETCH(long long, expectedDeltaData);
    QFETCH(int, expectSignals);

    configure(intDig, decDig);
    QVERIFY(index >= 0 && index < intDig + decDig);

    /* select the digit with arrow keys: each Key_Right moves the selection one
     * position further, starting from no selection — geometry independent */
    for (int k = 0; k <= index; k++)
        QTest::keyClick(m_num, Qt::Key_Right);

    /* the single up/down button pair shares the objectName layoutmember0 */
    QList<QPushButton *> btns = m_num->findChildren<QPushButton *>("layoutmember0");
    QCOMPARE(btns.size(), 2);

    /* self calibration: one click at a safe value tells which button is up */
    m_num->setValue(0.0);
    btns.at(0)->click();
    const double calib = m_num->value();
    QVERIFY2(calib != 0.0, "calibration click had no effect");
    QPushButton *upBtn = (calib > 0.0) ? btns.at(0) : btns.at(1);
    QPushButton *downBtn = (calib > 0.0) ? btns.at(1) : btns.at(0);

    m_num->setValue(startValue);
    const long long startData =
        numFixedPointFromDecimalString(numOracleDecimalString(startValue, decDig), decDig);
    QSignalSpy spy(m_num, SIGNAL(valueChanged(double)));
    QVERIFY(spy.isValid());

    (up ? upBtn : downBtn)->click();

    bool singleOk = true, parseOk = true;
    const QString disp = numReadDisplayedString(m_num, intDig, decDig, &singleOk);
    const long long actualData = numParseDisplayedData(disp, &parseOk);
    QVERIFY2(singleOk && parseOk && (actualData - startData) == expectedDeltaData,
             qPrintable(QString("digit %1 %2-click starting at %3 (intDig=%4, decDig=%5): "
                                "expected fixed-point delta %6, got %7 (display \"%8\")")
                            .arg(index).arg(up ? "up" : "down")
                            .arg(startValue, 0, 'g', 17).arg(intDig).arg(decDig)
                            .arg(expectedDeltaData).arg(actualData - startData).arg(disp)));
    if (expectSignals >= 0)
        QCOMPARE(spy.count(), expectSignals);
}
