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

#include "knobData.h"

/*
 * One simulated channel of the "internal" test plugin.
 *
 * A channel is defined through the channel name itself:
 *     internal://NAME
 *     internal://NAME.{"type":"double","mode":"counter","init":0,"step":1,
 *                      "period":1000,"min":0,"max":100,"loop":true,"nelm":1,
 *                      "units":"V","prec":2,"enums":["OFF","ON"],"value":"text"}
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

    // write access (pvSetValue / pvSetWave)
    void setValue(double rdata, qint32 idata, const QString &sdata);
    void setWave(const QVector<double> &values);

    // fills the edata part of kData according to the configured type and current value
    void fillKnobData(knobData *kData) const;

    // string matching the regex pattern for the given enumeration index (string channels)
    QString generatedString(qint64 index) const;
    // number of different strings the regex pattern can produce (0 = no pattern)
    qint64 combinations() const { return m_combinations; }

    // configuration, plain data (also convenient for the unit tests)
    short fieldtype;            // caType from knobDefines.h
    Mode mode;
    double init;
    double step;
    int periodMs;
    double minimum;
    double maximum;
    bool hasMinimum;
    bool hasMaximum;
    bool loop;
    int nelm;
    QString units;
    short precision;
    QStringList enums;
    QString text;
    QString regexPattern;

    // state
    double value;
    bool needsPublish;

private:
    double elementValue(int i) const;
    void lowHigh(double *low, double *high) const;
    static bool parseRegexPattern(const QString &pattern, QList<QStringList> *segments,
                                  qint64 *combinations, QString *errorString);

    QList<QStringList> m_segments;  // regex pattern broken into enumerable segments
    qint64 m_combinations;
    QVector<double> m_waveOverride;
    int m_elapsedMs;
    bool m_configured;
};

#endif // INTERNAL_CHANNEL_H
