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

#ifndef TST_CANUMERIC_H
#define TST_CANUMERIC_H

#include <canumeric.h>

#include "tst_numeric_suite.h"

#include <QObject>
#include <QTest>

/* Reliability tests for the double-value display of caNumeric / ENumeric,
 * shared test bodies in tst_numeric_suite.h */
class TestCaNumeric : public QObject, private NumericSuiteBase<caNumeric, ENumeric>
{
    Q_OBJECT
public:
    TestCaNumeric() = default;

private slots:
    void initTestCase();
    void init() { t_init(); }
    void cleanup() { t_cleanup(); }

    void helperSelfTest() { t_helperSelfTest(); }
    void roundTripValue_data() { t_roundTripValue_data(); }
    void roundTripValue() { t_roundTripValue(); }
    void displayIntegrity_data() { t_displayIntegrity_data(); }
    void displayIntegrity() { t_displayIntegrity(); }
    void displayedValueCorrectness_data() { t_displayedValueCorrectness_data(); }
    void displayedValueCorrectness() { t_displayedValueCorrectness(); }
    void precisionBoundary_data() { t_precisionBoundary_data(); }
    void precisionBoundary() { t_precisionBoundary(); }
    void outOfRangeValueIsIgnored() { t_outOfRangeValueIsIgnored(); }
    void limitsAreClampedToDisplayCapacity() { t_limitsAreClampedToDisplayCapacity(); }
    void enumericCtorDigitsOverflow() { t_ctorDigitsOverflow(); }
    void setDecDigitsRescaling_data() { t_setDecDigitsRescaling_data(); }
    void setDecDigitsRescaling() { t_setDecDigitsRescaling(); }
    void valueChangedEmissionSemantics() { t_valueChangedEmissionSemantics(); }
    void autoShiftOnChannelValue() { t_autoShiftOnChannelValue(); }
    void suppressUserInputOnUnrepresentableValue() { t_suppressUserInputOnUnrepresentableValue(); }
    void roundingColorsMarkDigitsBeyondPrecision() { t_roundingColorsMarkDigitsBeyondPrecision(); }

    /* caNumeric has one up/down button pair per digit — widget specific test */
    void incrementDecrementByButtons_data();
    void incrementDecrementByButtons();
};

#endif // TST_CANUMERIC_H
