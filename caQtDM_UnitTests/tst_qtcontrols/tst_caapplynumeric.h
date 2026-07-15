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

#ifndef TST_CAAPPLYNUMERIC_H
#define TST_CAAPPLYNUMERIC_H

#include <caapplynumeric.h>
#include <enumeric.h>

#include "tst_numeric_suite.h"

#include <QObject>
#include <QTest>

/* Reliability tests for the double-value display of caApplyNumeric /
 * EApplyNumeric (a composite around an embedded ENumeric), exactly the same
 * test bodies as for caNumeric (tst_numeric_suite.h). */
class TestCaApplyNumeric : public QObject, private NumericSuiteBase<caApplyNumeric, EApplyNumeric>
{
    Q_OBJECT
public:
    TestCaApplyNumeric() = default;

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
    void eapplynumericCtorDigitsOverflow() { t_ctorDigitsOverflow(); }
    void setDecDigitsRescaling_data() { t_setDecDigitsRescaling_data(); }
    void setDecDigitsRescaling() { t_setDecDigitsRescaling(); }
    void valueChangedEmissionSemantics() { t_valueChangedEmissionSemantics(); }
    void autoShiftOnChannelValue() { t_autoShiftOnChannelValue(); }
    void suppressUserInputOnUnrepresentableValue() { t_suppressUserInputOnUnrepresentableValue(); }
    void roundingColorsMarkDigitsBeyondPrecision() { t_roundingColorsMarkDigitsBeyondPrecision(); }
    void incrementDecrementByButtons_data() { t_incrementDecrementByButtonsPerDigit_data(); }
    void incrementDecrementByButtons() { t_incrementDecrementByButtonsPerDigit(); }
    void nanAndInfHandling() { t_nanAndInfHandling(); }
    void negativeZeroAndTinyValues() { t_negativeZeroAndTinyValues(); }
    void asymmetricLimits() { t_asymmetricLimits(); }
    void keyboardIncrementDecrement() { t_keyboardIncrementDecrement(); }
    void writeAccessBlocksInput() { t_writeAccessBlocksInput(); }
    void suppressBlocksInteraction() { t_suppressBlocksInteraction(); }
    void mouseLeaveKeepsUserValue() { t_mouseLeaveRevertsToChannelValue(false); }
    void signalArgumentOnIncrement() { t_signalArgumentOnIncrement(); }
    void decimalPointPosition() { t_decimalPointPosition(); }
    void libDigitPattern() { t_libDigitPattern(); }
    void autoShiftKeepsConfiguredLimits() { t_autoShiftKeepsConfiguredLimits(); }
    void autoShiftNegativeValues() { t_autoShiftNegativeValues(); }
    void fixedFormatDisablesAutoShift() { t_fixedFormatDisablesAutoShift(); }

    /* apply specific: the button emits clicked(double) with the displayed value */
    void applyButtonEmitsClicked();

private:
    /* key/mouse events must go to the embedded ENumeric, its event filter
     * handles the interaction */
    QWidget *inputTarget() override { return m_num->findChild<ENumeric *>(); }
};

#endif // TST_CAAPPLYNUMERIC_H
