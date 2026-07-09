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

#include "tst_caapplynumeric.h"

#include <clocale>

void TestCaApplyNumeric::initTestCase()
{
    // to avoid failing tests because of a comma as decimal separator
    setlocale(LC_NUMERIC, "C");
}

void TestCaApplyNumeric::applyButtonEmitsClicked()
{
    configure(3, 2);
    m_num->setValue(4.5);

    QSignalSpy spy(m_num, SIGNAL(clicked(double)));
    QVERIFY(spy.isValid());

    /* the apply button is the only push button not named layoutmember* */
    QPushButton *applyBtn = Q_NULLPTR;
    foreach (QPushButton *b, m_num->findChildren<QPushButton *>()) {
        if (!b->objectName().startsWith("layoutmember")) {
            applyBtn = b;
            break;
        }
    }
    QVERIFY2(applyBtn, "apply button not found");

    applyBtn->click();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toDouble(), 4.5);
}
