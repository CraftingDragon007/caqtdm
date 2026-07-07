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

#ifndef TST_NUMERIC_SUITE_H
#define TST_NUMERIC_SUITE_H

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>
#include <cfloat>
#include <cmath>
#include <cstdlib>

/* Shared reliability test suite for the fixed point digit widgets
 * (caNumeric/ENumeric and caSpinbox/SNumeric).
 * Mathematically generated values (pi, e, sqrt(2), rationals, sin/exp series,
 * powers of ten) are compared against pure integer/string oracles; failure
 * messages mark wrong digits with '^' and digits beyond double precision with '~'.
 */

/* mathematical constants with 25 significant digits, used as "true value" oracles */
static const char *const NUM_PI_STR    = "3.1415926535897932384626434";
static const char *const NUM_E_STR     = "2.7182818284590452353602875";
static const char *const NUM_SQRT2_STR = "1.4142135623730950488016887";
static const char *const NUM_LN2_STR   = "0.6931471805599453094172321";

/*
 * ---------------------------------------------------------------------------
 *  oracles — pure integer/string arithmetic, independent of the widgets
 * ---------------------------------------------------------------------------
 */

static inline long long numPow10ll(int n)
{
    long long r = 1;
    for (int i = 0; i < n; i++) r *= 10;
    return r;
}

/* correctly rounded, locale independent decimal representation of the double */
static inline QString numOracleDecimalString(double v, int decDig)
{
    return QString::number(v, 'f', decDig);
}

static inline long long numFixedPointFromDecimalString(const QString &s, int decDig,
                                                       bool *ok = Q_NULLPTR)
{
    if (ok) *ok = true;
    QString str = s.trimmed();
    bool neg = str.startsWith(QLatin1Char('-'));
    if (neg || str.startsWith(QLatin1Char('+'))) str.remove(0, 1);
    QString intPart = str.section(QLatin1Char('.'), 0, 0);
    QString fracPart = str.section(QLatin1Char('.'), 1, 1);
    if (intPart.isEmpty()) intPart = QLatin1String("0");
    while (fracPart.size() <= decDig) fracPart.append(QLatin1Char('0'));

    bool ok1 = true, ok2 = true;
    long long result = intPart.toLongLong(&ok1) * numPow10ll(decDig);
    if (decDig > 0) result += fracPart.left(decDig).toLongLong(&ok2);
    /* rounding half away from zero */
    if (fracPart.at(decDig).digitValue() >= 5) result += 1;
    if ((!ok1 || !ok2) && ok) *ok = false;
    return neg ? -result : result;
}

/* exact long division: the first fracDigits fractional digits of num/den */
static inline QString numDecimalExpansion(long long num, long long den, int fracDigits)
{
    QString out;
    long long r = num % den;
    for (int i = 0; i < fracDigits; i++) {
        r *= 10;
        out.append(QLatin1Char((char) ('0' + r / den)));
        r %= den;
    }
    return out;
}

/* multiply a decimal string by 10^shift (shift >= 0) — exact, no float involved */
static inline QString numShiftDecimalPoint(const QString &s, int shift)
{
    QString str = s;
    bool neg = str.startsWith(QLatin1Char('-'));
    if (neg) str.remove(0, 1);
    int p = str.indexOf(QLatin1Char('.'));
    if (p < 0) p = str.size(); else str.remove(p, 1);
    p += shift;
    while (str.size() < p) str.append(QLatin1Char('0'));
    QString res = str.left(p) + QLatin1String(".") + str.mid(p);
    if (res.startsWith(QLatin1Char('.'))) res.prepend(QLatin1Char('0'));
    if (res.endsWith(QLatin1Char('.'))) res.chop(1);
    /* strip leading zeros of the integer part, keep at least one digit */
    while (res.size() > 1 && res.at(0) == QLatin1Char('0') && res.at(1) != QLatin1Char('.'))
        res.remove(0, 1);
    return neg ? QLatin1String("-") + res : res;
}

static inline QString numExpectedDisplayString(long long data, int intDig, int decDig)
{
    long long mag = llabs(data);
    long long div = numPow10ll(decDig);
    QString intStr = QString::number(mag / div);
    if (intStr.size() < intDig) intStr.prepend(QString(intDig - intStr.size(), QLatin1Char('0')));
    /* leading zero suppression as done by the widgets: contiguous leading zeros
     * become blanks, the least significant integer position keeps its digit */
    for (int j = 0; j < intStr.size() - 1 && intStr.at(j) == QLatin1Char('0'); j++)
        intStr[j] = QLatin1Char(' ');
    QString out = (data < 0) ? QLatin1String("-") : QLatin1String("+");
    out += intStr;
    if (decDig > 0)
        out += QLatin1String(".") + QString("%1").arg(mag % div, decDig, 10, QLatin1Char('0'));
    return out;
}

static inline QString numReadDisplayedString(QWidget *w, int intDig, int decDig,
                                             bool *singleCharOk)
{
    *singleCharOk = true;
    QLabel *sign = w->findChild<QLabel *>("layoutmember+");
    QString signText = sign ? sign->text() : QString("?");
    if (signText.size() != 1) *singleCharOk = false;
    QString out = signText;
    for (int i = 0; i < intDig + decDig; i++) {
        if (i == intDig && decDig > 0) out += QLatin1String(".");
        QLabel *l = w->findChild<QLabel *>(QString("layoutmember%1").arg(i));
        QString t = l ? l->text() : QString("?");
        if (t.size() != 1) *singleCharOk = false;
        out += t;
    }
    return out;
}

static inline long long numParseDisplayedData(const QString &display, bool *ok)
{
    *ok = display.size() >= 2;
    if (!*ok) return 0;
    QChar signChar = display.at(0);
    QString digitsOnly = display.mid(1);
    digitsOnly.remove(QLatin1Char('.'));
    digitsOnly.replace(QLatin1Char(' '), QLatin1Char('0')); // blanks are suppressed leading zeros
    bool numOk = false;
    long long mag = digitsOnly.toLongLong(&numOk);
    if (!numOk || (signChar != QLatin1Char('+') && signChar != QLatin1Char('-'))) {
        *ok = false;
        return 0;
    }
    return (signChar == QLatin1Char('-')) ? -mag : mag;
}

/* error budget of the widget conversion in units of the last displayed digit */
static inline long long numIntegerTolerance(double v, int decDig, double kappa)
{
    return (long long) llround(kappa * fabs(v) * pow(10.0, (double) decDig) * DBL_EPSILON);
}

/* digit position p (0 = last decimal) is beyond double precision iff 10^p <= 2*tol */
static inline int numUnreliableDigitCount(long long tol)
{
    if (tol <= 0) return 0;
    return (int) floor(log10(2.0 * (double) tol)) + 1;
}

static inline QString numFormatDigitComparison(double v, int intDig, int decDig,
                                               const QString &expected, const QString &actual,
                                               long long tol)
{
    const int len = qMax(expected.size(), actual.size());
    const QString e = expected.leftJustified(len);
    const QString a = actual.leftJustified(len);
    QString carets(len, QLatin1Char(' '));
    QString tildes(len, QLatin1Char(' '));
    for (int i = 0; i < len; i++)
        if (e.at(i) != a.at(i)) carets[i] = QLatin1Char('^');
    const int nU = numUnreliableDigitCount(tol);
    int marked = 0;
    for (int i = len - 1; i >= 0 && marked < nU; --i) {
        const QChar c = e.at(i);
        if (c == QLatin1Char('.') || c == QLatin1Char('+') || c == QLatin1Char('-')) continue;
        tildes[i] = QLatin1Char('~');
        marked++;
    }
    QString msg;
    msg += QLatin1String("numeric display mismatch\n");
    msg += QString("  input value       : %1   (intDig=%2, decDig=%3)\n")
               .arg(v, 0, 'g', 17).arg(intDig).arg(decDig);
    msg += QLatin1String("  expected digits   : ") + e + QLatin1String("\n");
    msg += QLatin1String("  actual   digits   : ") + a + QLatin1String("\n");
    msg += QLatin1String("  wrong digits      : ") + carets + QLatin1String("\n");
    msg += QString("  beyond precision  : %1   (trailing %2 digit(s), integer tolerance T=%3)")
               .arg(tildes).arg(nU).arg(tol);
    return msg;
}

static const int s_numDigitConfigs[][2] = {
    {1, 0}, {2, 1}, {3, 2}, {5, 3}, {8, 6}, {9, 8},
    {10, 7}, {1, 16}, {2, 15}, {17, 1}, {9, 9}
};
static const int s_numDigitConfigCount =
    (int) (sizeof(s_numDigitConfigs) / sizeof(s_numDigitConfigs[0]));

/*
 * ---------------------------------------------------------------------------
 *  shared fixture and test bodies; WidgetT = ca widget, BaseT = its base class
 * ---------------------------------------------------------------------------
 */

template <class WidgetT, class BaseT>
class NumericSuiteBase
{
protected:
    NumericSuiteBase() : m_host(Q_NULLPTR), m_num(Q_NULLPTR) {}

    void t_init()
    {
        /* the widget must have a parent: the event filters dereference parent() */
        m_host = new QWidget();
        m_num = new WidgetT(m_host);
    }

    void t_cleanup()
    {
        delete m_host;
        m_host = Q_NULLPTR;
        m_num = Q_NULLPTR;
    }

    /* largest value the digit labels can represent: (10^(I+D) - 1) * 10^-D */
    double capacity(int intDig, int decDig) const
    {
        return (double) (numPow10ll(intDig + decDig) - 1) * pow(10.0, -(double) decDig);
    }

    void configure(int intDig, int decDig)
    {
        /* free the digit budget first: the widget clamps the total to 18 digits */
        m_num->setDecDigits(0);
        m_num->setIntDigits(intDig);
        m_num->setMaxValue(1.0);
        m_num->setMinValue(-1.0);
        m_num->setDecDigits(decDig);
        const double cap = capacity(intDig, decDig);
        m_num->setMaxValue(cap);
        m_num->setMinValue(-cap);
    }

    /* mathematically generated doubles covering the displayable range */
    QList<double> mathValues(int intDig, int decDig) const
    {
        const double cap = capacity(intDig, decDig);
        QList<double> candidates;
        candidates << 0.0 << 1.0 << -1.0
                   << QString(NUM_PI_STR).toDouble() << -QString(NUM_PI_STR).toDouble()
                   << QString(NUM_E_STR).toDouble()
                   << QString(NUM_SQRT2_STR).toDouble()
                   << QString(NUM_LN2_STR).toDouble()
                   << 1.0 / 3.0 << 2.0 / 3.0
                   << 0.1 + 0.2;
        for (int k = 1; k <= 6; k++) candidates << (double) k / 7.0;
        for (int k = 1; k <= 5; k++) candidates << sin((double) k);
        for (int k = 1; k <= 4; k++) candidates << exp((double) k);
        for (int k = -decDig; k < intDig; k++) {
            const double p = pow(10.0, (double) k);
            candidates << p << -p << p / 3.0 << -p / 3.0;
        }
        candidates << cap << -cap
                   << (double) (numPow10ll(intDig + decDig) - 2) * pow(10.0, -(double) decDig);

        /* keep only values whose correctly rounded fixed point representation
         * fits the display capacity (a double comparison against cap is not
         * exact: for 16+ digits cap itself rounds up beyond the capacity) */
        QList<double> result;
        const long long capData = numPow10ll(intDig + decDig) - 1;
        foreach (const double v, candidates) {
            bool ok = true;
            const long long fixed =
                numFixedPointFromDecimalString(numOracleDecimalString(v, decDig), decDig, &ok);
            if (ok && llabs(fixed) <= capData) result << v;
        }
        return result;
    }

    void addConfigValueColumns()
    {
        QTest::addColumn<int>("intDig");
        QTest::addColumn<int>("decDig");
        QTest::addColumn<double>("value");

        for (int c = 0; c < s_numDigitConfigCount; c++) {
            const int I = s_numDigitConfigs[c][0];
            const int D = s_numDigitConfigs[c][1];
            int n = 0;
            foreach (const double v, mathValues(I, D)) {
                QTest::newRow(qPrintable(QString("I%1.D%2.#%3 v=%4")
                                             .arg(I).arg(D).arg(n++).arg(v, 0, 'g', 17)))
                    << I << D << v;
            }
        }
    }

    /* verify the oracles against hard coded cases, so that an oracle bug is not
     * mistaken for a widget bug */
    void t_helperSelfTest()
    {
        QCOMPARE(numPow10ll(0), Q_INT64_C(1));
        QCOMPARE(numPow10ll(18), Q_INT64_C(1000000000000000000));

        QCOMPARE(numDecimalExpansion(1, 7, 12), QString("142857142857"));
        QCOMPARE(numDecimalExpansion(1, 3, 5), QString("33333"));

        QCOMPARE(numShiftDecimalPoint("3.14159", 3), QString("3141.59"));
        QCOMPARE(numShiftDecimalPoint("3.14", 5), QString("314000"));
        QCOMPARE(numShiftDecimalPoint("-0.5", 1), QString("-5"));
        QCOMPARE(numShiftDecimalPoint("0.05", 1), QString("0.5"));
        QCOMPARE(numShiftDecimalPoint("0.0625", 2), QString("6.25"));

        bool ok = false;
        QCOMPARE(numFixedPointFromDecimalString("12.5", 1, &ok), Q_INT64_C(125));
        QVERIFY(ok);
        QCOMPARE(numFixedPointFromDecimalString("12.34", 2), Q_INT64_C(1234));
        QCOMPARE(numFixedPointFromDecimalString("-3.14159", 2), Q_INT64_C(-314));
        QCOMPARE(numFixedPointFromDecimalString("2.675", 2), Q_INT64_C(268)); // half away
        QCOMPARE(numFixedPointFromDecimalString("-0.5", 0), Q_INT64_C(-1));   // half away
        QCOMPARE(numFixedPointFromDecimalString("1.9999", 0), Q_INT64_C(2));
        QCOMPARE(numFixedPointFromDecimalString("0.1", 3), Q_INT64_C(100));

        QCOMPARE(numExpectedDisplayString(125, 3, 1), QString("+ 12.5"));
        QCOMPARE(numExpectedDisplayString(0, 3, 1), QString("+  0.0"));
        QCOMPARE(numExpectedDisplayString(-905, 2, 1), QString("-90.5"));
        QCOMPARE(numExpectedDisplayString(5, 1, 0), QString("+5"));
        QCOMPARE(numExpectedDisplayString(13, 3, 0), QString("+ 13"));

        QCOMPARE(numParseDisplayedData(QString("+ 12.5"), &ok), Q_INT64_C(125));
        QVERIFY(ok);
        QCOMPARE(numParseDisplayedData(QString("-90.5"), &ok), Q_INT64_C(-905));
        QVERIFY(ok);
        QCOMPARE(numParseDisplayedData(QString("+  0.0"), &ok), Q_INT64_C(0));
        QVERIFY(ok);

        QCOMPARE(numUnreliableDigitCount(0), 0);
        QCOMPARE(numUnreliableDigitCount(1), 1);
        QCOMPARE(numUnreliableDigitCount(28), 2);
        QCOMPARE(numUnreliableDigitCount(500), 4);
    }

    void t_roundTripValue_data()
    {
        addConfigValueColumns();
    }

    void t_roundTripValue()
    {
        QFETCH(int, intDig);
        QFETCH(int, decDig);
        QFETCH(double, value);

        configure(intDig, decDig);
        m_num->setValue(value);

        bool oracleOk = true;
        const long long oracleData = numFixedPointFromDecimalString(
            numOracleDecimalString(value, decDig), decDig, &oracleOk);
        QVERIFY(oracleOk);
        const long long T = numIntegerTolerance(value, decDig, 4.0);
        const double expected = (double) oracleData * pow(10.0, -(double) decDig);
        const double tol = 4.0 * DBL_EPSILON * qMax(1.0, fabs(expected))
                           + (double) T * pow(10.0, -(double) decDig);
        const double actual = m_num->value();

        QVERIFY2(fabs(actual - expected) <= tol,
                 qPrintable(QString("value() round trip failed for %1 (intDig=%2, decDig=%3):\n"
                                    "  expected %4\n  actual   %5\n"
                                    "  |diff| %6 > tolerance %7 (integer tolerance T=%8)")
                                .arg(value, 0, 'g', 17).arg(intDig).arg(decDig)
                                .arg(expected, 0, 'g', 17).arg(actual, 0, 'g', 17)
                                .arg(fabs(actual - expected), 0, 'g', 6).arg(tol, 0, 'g', 6)
                                .arg(T)));
    }

    void t_displayIntegrity_data()
    {
        addConfigValueColumns();
        /* fixed point values beyond 2^53: exact only with integer digit decomposition */
        QTest::newRow("big I16.D1 pi*1e15")
            << 16 << 1 << numShiftDecimalPoint(NUM_PI_STR, 15).toDouble();
        QTest::newRow("big I17.D1 pi*1e16")
            << 17 << 1 << numShiftDecimalPoint(NUM_PI_STR, 16).toDouble();
        QTest::newRow("big I18.D0 pi*1e17")
            << 18 << 0 << numShiftDecimalPoint(NUM_PI_STR, 17).toDouble();
        QTest::newRow("big I9.D9 pi*1e8")
            << 9 << 9 << numShiftDecimalPoint(NUM_PI_STR, 8).toDouble();
    }

    void t_displayIntegrity()
    {
        QFETCH(int, intDig);
        QFETCH(int, decDig);
        QFETCH(double, value);

        configure(intDig, decDig);
        m_num->setValue(value);

        /* the labels must render exactly the correctly rounded fixed point value */
        bool oracleOk = true;
        const long long expectedStored = numFixedPointFromDecimalString(
            numOracleDecimalString(value, decDig), decDig, &oracleOk);
        QVERIFY(oracleOk);
        const QString expected = numExpectedDisplayString(expectedStored, intDig, decDig);
        bool singleOk = true;
        const QString actual = numReadDisplayedString(m_num, intDig, decDig, &singleOk);

        QVERIFY2(singleOk && actual == expected,
                 qPrintable(numFormatDigitComparison(value, intDig, decDig, expected, actual, 0)));
    }

    void t_displayedValueCorrectness_data()
    {
        addConfigValueColumns();
        /* decimal ties: correct rounding differs from a naive round(v * pow(10, D)) */
        QTest::newRow("tie 0.15@D1") << 2 << 1 << 0.15;
        QTest::newRow("tie 0.35@D1") << 2 << 1 << 0.35;
        QTest::newRow("tie 2.675@D2") << 3 << 2 << 2.675;
        QTest::newRow("tie 8.835@D2") << 3 << 2 << 8.835;
        QTest::newRow("tie 1.005@D2") << 3 << 2 << 1.005;
        QTest::newRow("tie 0.045@D2") << 3 << 2 << 0.045;
    }

    void t_displayedValueCorrectness()
    {
        QFETCH(int, intDig);
        QFETCH(int, decDig);
        QFETCH(double, value);

        configure(intDig, decDig);
        m_num->setValue(value);

        bool oracleOk = true;
        const long long oracleData = numFixedPointFromDecimalString(
            numOracleDecimalString(value, decDig), decDig, &oracleOk);
        QVERIFY(oracleOk);
        const long long T = numIntegerTolerance(value, decDig, 4.0);

        bool singleOk = true, parseOk = true;
        const QString actual = numReadDisplayedString(m_num, intDig, decDig, &singleOk);
        const long long displayed = numParseDisplayedData(actual, &parseOk);
        const QString expected = numExpectedDisplayString(oracleData, intDig, decDig);

        QVERIFY2(singleOk && parseOk && llabs(displayed - oracleData) <= T,
                 qPrintable(numFormatDigitComparison(value, intDig, decDig, expected, actual, T)));
    }

    void t_precisionBoundary_data()
    {
        QTest::addColumn<int>("intDig");
        QTest::addColumn<int>("decDig");
        QTest::addColumn<QString>("trueDigits");

        /* constants with 25 significant digits at increasing total digit counts:
         * beyond ~16 significant digits a double cannot hold the displayed digits */
        QTest::newRow("pi I2.D8") << 2 << 8 << QString(NUM_PI_STR);
        QTest::newRow("e I2.D8") << 2 << 8 << QString(NUM_E_STR);
        QTest::newRow("pi I2.D12") << 2 << 12 << QString(NUM_PI_STR);
        QTest::newRow("sqrt2 I2.D12") << 2 << 12 << QString(NUM_SQRT2_STR);
        QTest::newRow("pi I2.D14") << 2 << 14 << QString(NUM_PI_STR);
        QTest::newRow("pi I2.D15") << 2 << 15 << QString(NUM_PI_STR);
        QTest::newRow("e I2.D15") << 2 << 15 << QString(NUM_E_STR);
        QTest::newRow("pi I2.D16") << 2 << 16 << QString(NUM_PI_STR);
        QTest::newRow("1/3 I1.D16") << 1 << 16 << QString("0.") + numDecimalExpansion(1, 3, 25);
        QTest::newRow("2/7 I1.D16") << 1 << 16 << QString("0.") + numDecimalExpansion(2, 7, 25);
        QTest::newRow("ln2 I1.D16") << 1 << 16 << QString(NUM_LN2_STR);
        QTest::newRow("pi*1e4 I6.D12") << 6 << 12 << numShiftDecimalPoint(NUM_PI_STR, 4);
        QTest::newRow("pi*1e7 I9.D9") << 9 << 9 << numShiftDecimalPoint(NUM_PI_STR, 7);
        QTest::newRow("pi*1e10 I12.D6") << 12 << 6 << numShiftDecimalPoint(NUM_PI_STR, 10);
        QTest::newRow("pi*1e15 I17.D1") << 17 << 1 << numShiftDecimalPoint(NUM_PI_STR, 15);
        /* pi * 10^k sweep at a constant total digit count of 18 */
        for (int k = 0; k <= 7; k++) {
            QTest::newRow(qPrintable(QString("pi*1e%1 I%2.D%3").arg(k).arg(k + 2).arg(16 - k)))
                << (k + 2) << (16 - k) << numShiftDecimalPoint(NUM_PI_STR, k);
        }
    }

    void t_precisionBoundary()
    {
        QFETCH(int, intDig);
        QFETCH(int, decDig);
        QFETCH(QString, trueDigits);

        configure(intDig, decDig);
        const double v = trueDigits.toDouble();
        QVERIFY(fabs(v) <= capacity(intDig, decDig));
        m_num->setValue(v);

        bool oracleOk = true;
        const long long trueData = numFixedPointFromDecimalString(trueDigits, decDig, &oracleOk);
        QVERIFY(oracleOk);
        /* one extra unit for the representation error of the constant itself */
        const long long T = numIntegerTolerance(v, decDig, 4.0) + 1;

        bool singleOk = true, parseOk = true;
        const QString actual = numReadDisplayedString(m_num, intDig, decDig, &singleOk);
        const long long displayed = numParseDisplayedData(actual, &parseOk);
        const QString expected = numExpectedDisplayString(trueData, intDig, decDig);
        const QString report = numFormatDigitComparison(v, intDig, decDig, expected, actual, T);

        /* always make the double precision boundary visible in the test output */
        if (numUnreliableDigitCount(T) > 0)
            qInfo("true value %s:\n%s", qPrintable(trueDigits), qPrintable(report));

        QVERIFY2(singleOk && parseOk && llabs(displayed - trueData) <= T, qPrintable(report));
    }

    void t_outOfRangeValueIsIgnored()
    {
        configure(3, 2); // capacity 999.99
        m_num->setValue(12.34);
        bool singleOk = true;
        const QString before = numReadDisplayedString(m_num, 3, 2, &singleOk);
        QVERIFY(singleOk);

        QSignalSpy spy(m_num, SIGNAL(valueChanged(double)));
        QVERIFY(spy.isValid());
        m_num->setValue(1000.0);
        m_num->setValue(-1000.0);

        QVERIFY2(fabs(m_num->value() - 12.34) < 1e-9,
                 qPrintable(QString("out-of-range setValue changed the value to %1")
                                .arg(m_num->value(), 0, 'g', 17)));
        const QString after = numReadDisplayedString(m_num, 3, 2, &singleOk);
        QCOMPARE(after, before);
        QCOMPARE(spy.count(), 0);
    }

    void t_limitsAreClampedToDisplayCapacity()
    {
        /* default widget: 2 integer digits, 1 decimal digit, constructor limits
         * +-100000; the effective limits must be clamped to the display capacity */
        QSignalSpy spy(m_num, SIGNAL(valueChanged(double)));
        QVERIFY(spy.isValid());

        m_num->setValue(1234.5); /* beyond the 99.9 capacity: must be ignored */
        QVERIFY2(fabs(m_num->value()) < 1e-9,
                 qPrintable(QString("value beyond the display capacity was accepted: value() = %1")
                                .arg(m_num->value(), 0, 'g', 17)));
        QCOMPARE(spy.count(), 0);

        m_num->setValue(99.9); /* the capacity itself must be accepted */
        QVERIFY2(fabs(m_num->value() - 99.9) < 1e-9,
                 qPrintable(QString("capacity value 99.9 was rejected: value() = %1")
                                .arg(m_num->value(), 0, 'g', 17)));
        QCOMPARE(spy.count(), 1);

        /* with more integer digits the same value fits and must be displayed */
        m_num->setIntDigits(5);
        m_num->setMaxValue(100000.0);
        m_num->setValue(1234.5);
        QVERIFY2(fabs(m_num->value() - 1234.5) < 1e-9,
                 qPrintable(QString("1234.5 rejected after setIntDigits(5): value() = %1")
                                .arg(m_num->value(), 0, 'g', 17)));
        bool singleOk = true;
        const QString disp = numReadDisplayedString(m_num, 5, 1, &singleOk);
        QVERIFY2(singleOk && disp == numExpectedDisplayString(12345, 5, 1),
                 qPrintable(numFormatDigitComparison(1234.5, 5, 1,
                                                     numExpectedDisplayString(12345, 5, 1),
                                                     disp, 0)));
    }

    void t_ctorDigitsOverflow()
    {
        /* the base widget constructor must compute the fixed point limits with
         * integer arithmetic; an (int) cast of pow(10, digits) overflows for
         * digits >= 10 */
        BaseT e(m_host, 10, 0);
        e.setValue(5.0e9);
        const double got = e.value();
        QVERIFY2(fabs(got - 5.0e9) <= 1.0,
                 qPrintable(QString("base widget with 10 integer digits cannot take the value "
                                    "5e9: value() = %1 — constructor limits overflow")
                                .arg(got, 0, 'g', 17)));
    }

    void t_setDecDigitsRescaling_data()
    {
        QTest::addColumn<int>("intDig");
        QTest::addColumn<int>("startD");
        QTest::addColumn<int>("newD");
        QTest::addColumn<double>("startValue");
        QTest::addColumn<long long>("expectedData");

        QTest::newRow("increase D1->D3 keeps 12.5") << 2 << 1 << 3 << 12.5 << Q_INT64_C(12500);
        QTest::newRow("decrease D1->D0 must round 12.9 to 13")
            << 2 << 1 << 0 << 12.9 << Q_INT64_C(13);
        QTest::newRow("decrease D16->D15 pi")
            << 2 << 16 << 15 << QString(NUM_PI_STR).toDouble() << Q_INT64_C(3141592653589793);
    }

    void t_setDecDigitsRescaling()
    {
        QFETCH(int, intDig);
        QFETCH(int, startD);
        QFETCH(int, newD);
        QFETCH(double, startValue);
        QFETCH(long long, expectedData);

        configure(intDig, startD);
        m_num->setValue(startValue);
        m_num->setDecDigits(newD);

        bool singleOk = true, parseOk = true;
        const QString disp = numReadDisplayedString(m_num, intDig, newD, &singleOk);
        const long long actualData = numParseDisplayedData(disp, &parseOk);
        QVERIFY2(singleOk && parseOk && actualData == expectedData,
                 qPrintable(QString("setDecDigits(%1 -> %2) starting from %3: expected "
                                    "fixed-point data %4, widget shows \"%5\" (data %6) — "
                                    "the rescaling must round half away from zero")
                                .arg(startD).arg(newD).arg(startValue, 0, 'g', 17)
                                .arg(expectedData).arg(disp).arg(actualData)));

        const double expVal = (double) expectedData * pow(10.0, -(double) newD);
        QVERIFY2(fabs(m_num->value() - expVal) <= 4.0 * DBL_EPSILON * qMax(1.0, fabs(expVal)),
                 qPrintable(QString("value() after setDecDigits is %1, expected %2")
                                .arg(m_num->value(), 0, 'g', 17).arg(expVal, 0, 'g', 17)));

        /* the limits must still work after the rescaling */
        m_num->setValue(1.5);
        const QString disp2 = numReadDisplayedString(m_num, intDig, newD, &singleOk);
        const long long data2 = numParseDisplayedData(disp2, &parseOk);
        QVERIFY2(singleOk && parseOk && data2 == numFixedPointFromDecimalString("1.5", newD),
                 qPrintable(QString("setValue(1.5) after setDecDigits(%1) shows \"%2\"")
                                .arg(newD).arg(disp2)));
    }

    void t_valueChangedEmissionSemantics()
    {
        configure(3, 2); // capacity 999.99

        QSignalSpy spy(m_num, SIGNAL(valueChanged(double)));
        QVERIFY(spy.isValid());

        m_num->setValue(1.25);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toDouble(), m_num->value());

        m_num->setValue(1.25); // unchanged value: no signal
        QCOMPARE(spy.count(), 0);

        m_num->setValue(5000.0); // out of range: silently ignored, no signal
        QCOMPARE(spy.count(), 0);

        m_num->silentSetValue(7.5); // changes the value but must not emit
        QCOMPARE(spy.count(), 0);
        QVERIFY2(fabs(m_num->value() - 7.5) < 1e-9,
                 qPrintable(QString("silentSetValue(7.5) resulted in value() = %1")
                                .arg(m_num->value(), 0, 'g', 17)));
    }

    void t_autoShiftOnChannelValue()
    {
        /* channel values (silentSetValue) may shift decimal digits into integer
         * digits when the value needs them, and shift back when it shrinks again;
         * user values (setValue) must never change the configuration */
        configure(2, 4);
        QSignalSpy spy(m_num, SIGNAL(valueChanged(double)));
        QVERIFY(spy.isValid());

        m_num->silentSetValue(12345.6); /* needs 5 integer digits */
        QCOMPARE(m_num->intDigits(), 5);
        QCOMPARE(m_num->decDigits(), 1);
        QVERIFY2(fabs(m_num->value() - 12345.6) < 1e-9,
                 qPrintable(QString("value() after auto shift is %1")
                                .arg(m_num->value(), 0, 'g', 17)));
        bool singleOk = true;
        QString disp = numReadDisplayedString(m_num, 5, 1, &singleOk);
        QVERIFY2(singleOk && disp == numExpectedDisplayString(123456, 5, 1),
                 qPrintable(numFormatDigitComparison(12345.6, 5, 1,
                                                     numExpectedDisplayString(123456, 5, 1),
                                                     disp, 0)));

        m_num->setValue(3.5); /* user value: the configuration must stay (5,1) */
        QCOMPARE(m_num->intDigits(), 5);
        QCOMPARE(m_num->decDigits(), 1);
        QVERIFY(fabs(m_num->value() - 3.5) < 1e-9);

        m_num->silentSetValue(1.25); /* small again: back to the original (2,4) */
        QCOMPARE(m_num->intDigits(), 2);
        QCOMPARE(m_num->decDigits(), 4);
        QVERIFY2(fabs(m_num->value() - 1.25) < 1e-9,
                 qPrintable(QString("value() after shifting back is %1")
                                .arg(m_num->value(), 0, 'g', 17)));
        disp = numReadDisplayedString(m_num, 2, 4, &singleOk);
        QVERIFY2(singleOk && disp == numExpectedDisplayString(12500, 2, 4),
                 qPrintable(numFormatDigitComparison(1.25, 2, 4,
                                                     numExpectedDisplayString(12500, 2, 4),
                                                     disp, 0)));

        QCOMPARE(spy.count(), 1); /* only the user setValue may emit */
    }

    void t_suppressUserInputOnUnrepresentableValue()
    {
        configure(3, 2);
        m_num->silentSetValue(1.0e19); /* does not fit any digit configuration */

        QLabel *first = m_num->template findChild<QLabel *>("layoutmember0");
        QLabel *last = m_num->template findChild<QLabel *>("layoutmember4");
        QLabel *sign = m_num->template findChild<QLabel *>("layoutmember+");
        QVERIFY(first && last && sign);
        QCOMPARE(first->text(), QString("*"));
        QCOMPARE(last->text(), QString("*"));
        QCOMPARE(sign->text(), QString(""));

        m_num->silentSetValue(42.5); /* processable again: the display must recover */
        bool singleOk = true;
        const QString disp = numReadDisplayedString(m_num, 3, 2, &singleOk);
        QVERIFY2(singleOk && disp == numExpectedDisplayString(4250, 3, 2),
                 qPrintable(numFormatDigitComparison(42.5, 3, 2,
                                                     numExpectedDisplayString(4250, 3, 2),
                                                     disp, 0)));
        QVERIFY(fabs(m_num->value() - 42.5) < 1e-9);
    }

    void t_roundingColorsMarkDigitsBeyondPrecision()
    {
        /* shown significant digits beyond the 15 digit precision threshold are
         * colored with the rounding color (light gray) */
        configure(9, 9);
        m_num->setValue(numShiftDecimalPoint(QString(NUM_PI_STR), 8).toDouble());
        for (int i = 0; i < 18; i++) {
            QLabel *l = m_num->template findChild<QLabel *>(QString("layoutmember%1").arg(i));
            QVERIFY(l);
            if (i >= 15) { /* 18 shown digits: the last three exceed the threshold */
                QVERIFY2(l->styleSheet().startsWith("QLabel {color:"),
                         qPrintable(QString("label %1 should carry the rounding color, "
                                            "stylesheet is \"%2\"").arg(i).arg(l->styleSheet())));
            } else {
                QVERIFY2(l->styleSheet().isEmpty(),
                         qPrintable(QString("label %1 unexpectedly colored: \"%2\"")
                                        .arg(i).arg(l->styleSheet())));
            }
        }

        /* a small value on the same widget must clear the previous coloring */
        m_num->setValue(1.5);
        for (int i = 0; i < 18; i++) {
            QLabel *l = m_num->template findChild<QLabel *>(QString("layoutmember%1").arg(i));
            QVERIFY(l);
            QVERIFY2(l->styleSheet().isEmpty(),
                     qPrintable(QString("label %1 still colored after a small value: \"%2\"")
                                    .arg(i).arg(l->styleSheet())));
        }

        /* few digits: nothing to color */
        configure(3, 2);
        m_num->setValue(42.5);
        for (int i = 0; i < 5; i++) {
            QLabel *l = m_num->template findChild<QLabel *>(QString("layoutmember%1").arg(i));
            QVERIFY(l);
            QVERIFY(l->styleSheet().isEmpty());
        }

        /* label indices beyond 15 but only two significant digits: no coloring */
        configure(17, 1);
        m_num->setValue(1.5);
        for (int i = 0; i < 18; i++) {
            QLabel *l = m_num->template findChild<QLabel *>(QString("layoutmember%1").arg(i));
            QVERIFY(l);
            QVERIFY2(l->styleSheet().isEmpty(),
                     qPrintable(QString("label %1 colored by index instead of significance: "
                                        "\"%2\"").arg(i).arg(l->styleSheet())));
        }
    }

    QWidget *m_host;
    WidgetT *m_num;
};

#endif // TST_NUMERIC_SUITE_H
