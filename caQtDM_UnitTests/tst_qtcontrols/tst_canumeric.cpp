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

#include "tst_canumeric.h"

#include <QLineEdit>
#include <clocale>

void TestCaNumeric::initTestCase()
{
    // to avoid failing tests because of a comma as decimal separator
    setlocale(LC_NUMERIC, "C");
}

void TestCaNumeric::lineEditInput()
{
    configure(3, 2);
    m_num->setValue(1.0);

    /* a double click opens the direct input line edit */
    QTest::mouseDClick(m_num, Qt::LeftButton);
    QLineEdit *edit = m_num->findChild<QLineEdit *>();
    QVERIFY2(edit, "double click must create the input line edit");

    edit->setText("13.5");
    QTest::keyClick(edit, Qt::Key_Return);
    QVERIFY2(fabs(m_num->value() - 13.5) < 1e-9,
             qPrintable(QString("typed 13.5, value() = %1").arg(m_num->value(), 0, 'g', 17)));

    QTest::mouseDClick(m_num, Qt::LeftButton);
    edit->setText("abc"); /* not a number */
    QTest::keyClick(edit, Qt::Key_Return);
    QVERIFY2(fabs(m_num->value() - 13.5) < 1e-9, "invalid text must be ignored");

    QTest::mouseDClick(m_num, Qt::LeftButton);
    edit->setText("1,5"); /* comma is not accepted, parsing is C locale */
    QTest::keyClick(edit, Qt::Key_Return);
    QVERIFY2(fabs(m_num->value() - 13.5) < 1e-9, "comma input must be ignored");

    QTest::mouseDClick(m_num, Qt::LeftButton);
    edit->setText("5000"); /* beyond the limits */
    QTest::keyClick(edit, Qt::Key_Return);
    QVERIFY2(fabs(m_num->value() - 13.5) < 1e-9, "out of range input must be ignored");
}
