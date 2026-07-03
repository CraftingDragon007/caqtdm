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
 *  Contact details:
 *    helge.brands@psi.ch
 */
#ifndef INTERNAL_CHANNEL_H
#define INTERNAL_CHANNEL_H

#include <QString>
#include <QStringList>
#include <QVector>

#include <stdint.h>

#include "knobData.h"

// a limit coming from the JSON configuration that may or may not be defined;
// reads nicer than parallel has* flags
struct OptionalLimit
{
    double value;
    bool defined;

    OptionalLimit() : value(0.0), defined(false) {}
    void set(double newValue) { value = newValue; defined = true; }
};

/*
 * The current value in its native EPICS type, following the types the epics3
 * plugin works with: dbr_short_t (caINT), dbr_long_t (caLONG), dbr_float_t
 * (caFLOAT), dbr_double_t (caDOUBLE), dbr_enum_t (caENUM), dbr_char_t (caCHAR).
 * Only the member matching the channel fieldtype is used, so type specific
 * effects (int16 wrap around, float rounding, unsigned enum/char indexes)
 * behave like with a real control system and can be tested.
 */
struct NativeValue
{
    qint16  int16Value;   // caINT   (dbr_short_t)
    qint32  int32Value;   // caLONG  (dbr_long_t)
    float   floatValue;   // caFLOAT (dbr_float_t)
    double  doubleValue;  // caDOUBLE (dbr_double_t), also the index of string regex channels
    quint16 enumValue;    // caENUM  (dbr_enum_t)
    quint8  charValue;    // caCHAR  (dbr_char_t)

    NativeValue()
        : int16Value(0), int32Value(0), floatValue(0.0f), doubleValue(0.0)
        , enumValue(0), charValue(0) {}
};

/*
 * One simulated channel of the "internal" test plugin.
 *
 * A channel is defined through the channel name itself, using the EPICS field
 * names VAL, DRVL/DRVH (drive limits) and LOW/LOLO/HIGH/HIHI (alarm limits):
 *     internal://NAME
 *     internal://NAME.{"type":"double","mode":"counter","val":0,"step":1,
 *                      "period":1000,"drvl":0,"drvh":100,"loop":true,
 *                      "nelm":1,"nord":1,
 *                      "low":20,"lolo":10,"high":80,"hihi":90,
 *                      "units":"V","prec":2,"enums":["OFF","ON"],
 *                      "persistent":false}
 * A channel is dropped when its last monitor disappears (reference counting),
 * unless "persistent" is true: then it keeps running inside the process (a
 * counter simply continues) and displays can re-attach to the live value.
 * Arrays follow the EPICS waveform record: NELM is the maximum array size,
 * NORD the number of elements actually used (defaults to NELM, updated by
 * waveform writes). A waveform is initialized by giving "val" as an array,
 * which also defines NORD unless it is set explicitly:
 *     internal://WAVE.{"type":"double","nelm":8,"val":[1.5,2.5,3.5]}
 *     internal://TEXTS.{"type":"string","nelm":4,"val":["one","two"]}
 * For string channels "val" carries the fixed text. When alarm limits are
 * given, severity and status are set like an EPICS record would do, so alarm
 * colors of widgets can be exercised and tested.
 *
 * This class holds the JSON configuration, the counter state and knows how to
 * fill a knobData structure for every EPICS data type. It has no timer and no
 * GUI dependency, so it can be fully unit tested.
 *
 * String channels do not count; instead they can enumerate strings that match
 * a simple regular expression subset given as "regex":
 *     internal://MSG.{"type":"string","mode":"counter","regex":"STATE-[0-9]{2}"}
 * generates STATE-00, STATE-01, ... STATE-99 and wraps around. Supported are
 * literals, character classes like [a-z0-9] with an optional {n} quantifier
 * and flat alternation groups like (ON|OFF).
 */
class InternalChannel
{
public:
    enum Mode { Constant = 0, Counter };

    InternalChannel();

    // splits "NAME.{json}" into the two parts
    static QString baseName(const QString &pv);
    static QString jsonPart(const QString &pv);

    // parses the JSON configuration; on error the previous state stays untouched
    bool configure(const QString &json, QString *errorString = Q_NULLPTR);
    bool isConfigured() const { return m_configured; }

    // accumulates elapsed time and executes counter steps; returns true when the value changed
    bool advance(int elapsedMs);
    // one single counter step, independent of the period (used by advance and the tests)
    void tick();

    // the native value converted to double / stored from double (with the
    // truncation and wrap around of the native EPICS type)
    double currentValue() const;
    void setCurrentValue(double newValue);

    // write access (pvSetValue / pvSetWave)
    void setValue(double rdata, qint32 idata, const QString &sdata);
    void setWave(const QVector<double> &values);

    // fills the edata part of kData according to the configured type and current value
    void fillKnobData(knobData *kData) const;

    // current alarm severity/status for the given value, derived from low/lolo/high/hihi
    void alarmState(double checkValue, short *severity, short *status) const;

    // string matching the regex pattern for the given enumeration index (string channels)
    QString generatedString(qint64 index) const;
    // number of different strings the regex pattern can produce (0 = no pattern)
    qint64 combinations() const { return m_combinations; }

    // configuration, plain data (also convenient for the unit tests), EPICS field names
    short fieldtype;            // caType from knobDefines.h
    Mode mode;
    double val;                 // VAL: initial value
    double step;
    int periodMs;
    OptionalLimit drvl;         // DRVL/DRVH: drive limits (counter range, display/control limits)
    OptionalLimit drvh;
    OptionalLimit low;          // LOW/LOLO/HIGH/HIHI: alarm limits
    OptionalLimit lolo;
    OptionalLimit high;
    OptionalLimit hihi;
    bool loop;
    bool persistent;            // keeps the channel alive without any monitor
    int nelm;                   // NELM: maximum array size
    int nord;                   // NORD: number of elements actually used (<= nelm)
    QString units;
    short precision;
    QStringList enums;
    QString text;
    QStringList textArray;      // string waveform content given as "val" array
    QString regexPattern;

    // state
    NativeValue native;
    bool needsPublish;

private:
    double elementValue(int i) const;
    void counterRange(double *rangeLow, double *rangeHigh) const;
    static bool parseRegexPattern(const QString &pattern, QList<QStringList> *segments,
                                  qint64 *combinations, QString *errorString);

    QList<QStringList> m_segments;  // regex pattern broken into enumerable segments
    qint64 m_combinations;
    QVector<double> m_waveOverride;
    int m_elapsedMs;
    bool m_configured;
};

#endif // INTERNAL_CHANNEL_H
