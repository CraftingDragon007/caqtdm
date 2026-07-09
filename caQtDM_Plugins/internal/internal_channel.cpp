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
#include "internal_channel.h"
#include "alarmdefs.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QtGlobal>
#include <QtMath>

#include <stdlib.h>
#include <string.h>

InternalChannel::InternalChannel()
    : fieldtype(caDOUBLE)
    , mode(Constant)
    , val(0.0)
    , step(1.0)
    , periodMs(1000)
    , drvl()
    , drvh()
    , low()
    , lolo()
    , high()
    , hihi()
    , overflow(true)
    , persistent(false)
    , nelm(1)
    , nord(1)
    , units("")
    , precision(2)
    , enums()
    , text("")
    , regexPattern("")
    , native()
    , needsPublish(true)
    , severity(NO_ALARM)
    , status(0)
    , m_segments()
    , m_combinations(0)
    , m_regexReplace("")
    , m_waveOverride()
    , m_elapsedMs(0)
    , m_configured(false)
{
}

QString InternalChannel::jsonPart(const QString &pv)
{
    int pos = pv.indexOf(".{");
    if(pos == -1) return QString();
    return pv.mid(pos + 1).trimmed();
}

static bool parseFieldName(const QString &name, InternalChannel::Field *field)
{
    QString upper = name.toUpper();
    if(upper == "VAL")       *field = InternalChannel::FieldVal;
    else if(upper == "SEVR") *field = InternalChannel::FieldSevr;
    else if(upper == "STAT") *field = InternalChannel::FieldStat;
    else if(upper == "LOW")  *field = InternalChannel::FieldLow;
    else if(upper == "LOLO") *field = InternalChannel::FieldLolo;
    else if(upper == "HIGH") *field = InternalChannel::FieldHigh;
    else if(upper == "HIHI") *field = InternalChannel::FieldHihi;
    else if(upper == "DRVL") *field = InternalChannel::FieldDrvl;
    else if(upper == "DRVH") *field = InternalChannel::FieldDrvh;
    else if(upper == "PREC") *field = InternalChannel::FieldPrec;
    else if(upper == "EGU")  *field = InternalChannel::FieldEgu;
    else if(upper == "NELM") *field = InternalChannel::FieldNelm;
    else if(upper == "NORD") *field = InternalChannel::FieldNord;
    else return false;
    return true;
}

bool InternalChannel::splitField(const QString &pv, QString *base, Field *field)
{
    *field = FieldVal;
    base->clear();

    QString name = pv;
    int jsonPos = name.indexOf(".{");
    if(jsonPos != -1) name = name.left(jsonPos);
    name = name.trimmed();

    int dotPos = name.indexOf('.');
    if(dotPos > 0) {
        if(!parseFieldName(name.mid(dotPos + 1), field)) return false;
        name = name.left(dotPos);
    }
    *base = name;
    return true;
}

QString InternalChannel::baseName(const QString &pv)
{
    QString base;
    Field field;
    if(!splitField(pv, &base, &field)) return QString();
    return base;
}

// breaks a regex subset (literals, [a-z0-9]{n} classes, flat (A|B) groups)
// into segments whose combinations can be enumerated deterministically
bool InternalChannel::parseRegexPattern(const QString &pattern, QList<QStringList> *segments,
                                        qint64 *combinations, QString *errorString)
{
    segments->clear();
    QString literal;
    int i = 0;

    while(i < pattern.length()) {
        QChar c = pattern.at(i);

        if(c == '[' || c == '(') {
            if(!literal.isEmpty()) {
                segments->append(QStringList() << literal);
                literal.clear();
            }
        }

        if(c == '[') {
            int end = i + 1;
            QString chars;
            while(end < pattern.length() && pattern.at(end) != ']') {
                if(pattern.at(end) == '\\' && end + 1 < pattern.length()) {
                    chars.append(pattern.at(end + 1));
                    end += 2;
                } else if(end + 2 < pattern.length() && pattern.at(end + 1) == '-'
                          && pattern.at(end + 2) != ']') {
                    // character range like a-z
                    ushort from = pattern.at(end).unicode();
                    ushort to = pattern.at(end + 2).unicode();
                    if(from > to) {
                        if(errorString != Q_NULLPTR) *errorString = QString("invalid range in '%1'").arg(pattern);
                        return false;
                    }
                    for(ushort u = from; u <= to; u++) chars.append(QChar(u));
                    end += 3;
                } else {
                    chars.append(pattern.at(end));
                    end++;
                }
            }
            if(end >= pattern.length() || chars.isEmpty()) {
                if(errorString != Q_NULLPTR) *errorString = QString("unterminated or empty character class in '%1'").arg(pattern);
                return false;
            }
            i = end + 1;

            // optional {n} quantifier repeats the character class
            int repeat = 1;
            if(i < pattern.length() && pattern.at(i) == '{') {
                int close = pattern.indexOf('}', i);
                bool ok = false;
                if(close != -1) repeat = pattern.mid(i + 1, close - i - 1).toInt(&ok);
                if(!ok || repeat < 1) {
                    if(errorString != Q_NULLPTR) *errorString = QString("invalid quantifier in '%1'").arg(pattern);
                    return false;
                }
                i = close + 1;
            }

            QStringList options;
            foreach(const QChar &optionChar, chars) options.append(QString(optionChar));
            for(int r = 0; r < repeat; r++) segments->append(options);

        } else if(c == '(') {
            int close = pattern.indexOf(')', i);
            if(close == -1) {
                if(errorString != Q_NULLPTR) *errorString = QString("unterminated group in '%1'").arg(pattern);
                return false;
            }
            QStringList options = pattern.mid(i + 1, close - i - 1).split('|');
            if(options.isEmpty() || pattern.mid(i + 1, close - i - 1).isEmpty()) {
                if(errorString != Q_NULLPTR) *errorString = QString("empty group in '%1'").arg(pattern);
                return false;
            }
            segments->append(options);
            i = close + 1;

        } else if(c == '\\' && i + 1 < pattern.length()) {
            literal.append(pattern.at(i + 1));
            i += 2;

        } else {
            literal.append(c);
            i++;
        }
    }

    if(!literal.isEmpty()) segments->append(QStringList() << literal);
    if(segments->isEmpty()) {
        if(errorString != Q_NULLPTR) *errorString = "empty regex pattern";
        return false;
    }

    qint64 count = 1;
    foreach(const QStringList &options, *segments) {
        count *= options.size();
        if(count > Q_INT64_C(1000000000000)) { // keep the index within double precision
            if(errorString != Q_NULLPTR) *errorString = QString("regex '%1' allows too many combinations").arg(pattern);
            return false;
        }
    }
    *combinations = count;
    return true;
}

QString InternalChannel::generatedString(qint64 index) const
{
    if(m_combinations <= 0) return text;
    index %= m_combinations;
    if(index < 0) index += m_combinations;

    // mixed radix decomposition, the last segment changes fastest
    QString result;
    qint64 divisor = m_combinations;
    foreach(const QStringList &options, m_segments) {
        divisor /= options.size();
        result.append(options.at((int) (index / divisor)));
        index %= divisor;
    }
    return result;
}

bool InternalChannel::configure(const QString &json, QString *errorString)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if(parseError.error != QJsonParseError::NoError) {
        if(errorString != Q_NULLPTR) *errorString = parseError.errorString();
        return false;
    }
    if(!document.isObject()) {
        if(errorString != Q_NULLPTR) *errorString = "configuration is not a JSON object";
        return false;
    }
    QJsonObject object = document.object();

    if(object.contains("type")) {
        QString type = object["type"].toString().toLower();
        if(type == "double")      fieldtype = caDOUBLE;
        else if(type == "float")  fieldtype = caFLOAT;
        else if(type == "int")    fieldtype = caINT;
        else if(type == "long")   fieldtype = caLONG;
        else if(type == "enum")   fieldtype = caENUM;
        else if(type == "string") fieldtype = caSTRING;
        else if(type == "char")   fieldtype = caCHAR;
        else {
            if(errorString != Q_NULLPTR) *errorString = QString("unknown type '%1'").arg(type);
            return false;
        }
    }

    if(object.contains("mode")) {
        QString modeString = object["mode"].toString().toLower();
        if(modeString == "counter")       mode = Counter;
        else if(modeString == "constant") mode = Constant;
        else {
            if(errorString != Q_NULLPTR) *errorString = QString("unknown mode '%1'").arg(modeString);
            return false;
        }
    }

    // EPICS field names: VAL, DRVL/DRVH, LOW/LOLO/HIGH/HIHI
    if(object.contains("val")) {
        if(object["val"].isArray()) {
            // an array initializes the waveform content (numbers or strings)
            m_waveOverride.clear();
            textArray.clear();
            foreach(const QJsonValue &item, object["val"].toArray()) {
                if(item.isString()) textArray.append(item.toString());
                else                m_waveOverride.append(item.toDouble());
            }
            if(!m_waveOverride.isEmpty()) val = m_waveOverride.at(0);
        }
        // for string channels VAL carries the text, otherwise the initial value
        else if(object["val"].isString()) text = object["val"].toString();
        else                              val = object["val"].toDouble();
    }
    if(object.contains("step"))   step = object["step"].toDouble();
    if(object.contains("period")) periodMs = qMax(10, (int) object["period"].toDouble());
    if(object.contains("drvl"))   drvl.set(object["drvl"].toDouble());
    if(object.contains("drvh"))   drvh.set(object["drvh"].toDouble());
    if(object.contains("low"))    low.set(object["low"].toDouble());
    if(object.contains("lolo"))   lolo.set(object["lolo"].toDouble());
    if(object.contains("high"))   high.set(object["high"].toDouble());
    if(object.contains("hihi"))   hihi.set(object["hihi"].toDouble());
    if(object.contains("overflow"))   overflow = object["overflow"].toBool();
    if(object.contains("persistent")) persistent = object["persistent"].toBool();
    if(object.contains("nelm"))   nelm = qMax(1, (int) object["nelm"].toDouble());
    // NORD defaults to NELM (full array) and can never exceed it; an array
    // initialisation through "val" defines NORD unless it is given explicitly
    int initLength = qMax(m_waveOverride.size(), textArray.size());
    if(object.contains("nord"))   nord = qBound(0, (int) object["nord"].toDouble(), nelm);
    else if(initLength > 0)       nord = qMin(initLength, nelm);
    else                          nord = nelm;
    if(object.contains("units"))  units = object["units"].toString();
    if(object.contains("prec"))   precision = (short) object["prec"].toDouble();

    if(object.contains("enums")) {
        enums.clear();
        foreach(const QJsonValue &item, object["enums"].toArray()) {
            enums.append(item.toString());
        }
    }
    if(fieldtype == caENUM && enums.isEmpty()) enums << "OFF" << "ON";
    if(fieldtype != caDOUBLE && fieldtype != caFLOAT && !object.contains("prec")) precision = 0;

    if(object.contains("regex")) {
        QString pattern = object["regex"].toString();
        if(mode == Counter) {
            // counter: enumerable subset drives the generator
            QList<QStringList> segments;
            qint64 count = 0;
            if(!parseRegexPattern(pattern, &segments, &count, errorString)) return false;
            m_segments = segments;
            m_combinations = count;
        } else {
            // constant: written strings are processed with the full regex syntax,
            // the configured val is the replacement template (like the macro modification)
            if(!QRegularExpression(pattern).isValid()) {
                if(errorString != Q_NULLPTR) *errorString = QString("invalid regex '%1'").arg(pattern);
                return false;
            }
            m_regexReplace = text;
        }
        regexPattern = pattern;
    }

    setCurrentValue(val);
    updateAlarmState();
    m_elapsedMs = 0;
    needsPublish = true;
    m_configured = true;
    return true;
}

double InternalChannel::currentValue() const
{
    switch(fieldtype) {
    case caINT:   return (double) native.int16Value;
    case caLONG:  return (double) native.int32Value;
    case caFLOAT: return (double) native.floatValue;
    case caENUM:  return (double) native.enumValue;
    case caCHAR:  return (double) native.charValue;
    default:      return native.doubleValue;   // caDOUBLE and string regex index
    }
}

void InternalChannel::setCurrentValue(double newValue)
{
    // storing truncates and wraps like the native EPICS type would do
    switch(fieldtype) {
    case caINT:   native.int16Value = (qint16) (qint64) newValue; break;
    case caLONG:  native.int32Value = (qint32) (qint64) newValue; break;
    case caFLOAT: native.floatValue = (float) newValue; break;
    case caENUM:  native.enumValue = (quint16) (qint64) newValue; break;
    case caCHAR:  native.charValue = (quint8) (qint64) newValue; break;
    default:      native.doubleValue = newValue; break;
    }
}

void InternalChannel::counterRange(double *rangeLow, double *rangeHigh) const
{
    *rangeLow = drvl.defined ? drvl.value : -qInf();
    *rangeHigh = drvh.defined ? drvh.value : qInf();
    // an enum cycles through its states unless explicit drive limits are given
    if(fieldtype == caENUM && !enums.isEmpty()) {
        if(!drvl.defined) *rangeLow = 0.0;
        if(!drvh.defined) *rangeHigh = enums.size() - 1;
    }
    // a string channel with a regex pattern cycles through all combinations
    if(fieldtype == caSTRING && m_combinations > 0) {
        if(!drvl.defined) *rangeLow = 0.0;
        if(!drvh.defined) *rangeHigh = (double) (m_combinations - 1);
    }
}

// advances a value by one step and wraps (or saturates) at the range limits
static double steppedValue(double value, double step, double rangeLow, double rangeHigh, bool overflow)
{
    double next = value + step;
    if(next > rangeHigh)     next = overflow ? rangeLow : rangeHigh;
    else if(next < rangeLow) next = overflow ? rangeHigh : rangeLow;
    return next;
}

void InternalChannel::tick()
{
    if(mode != Counter) return;

    // a disconnected or invalid channel does not deliver new values
    if(severity == INVALID_ALARM || severity == NOTCONNECTED) return;

    double rangeLow, rangeHigh;
    counterRange(&rangeLow, &rangeHigh);

    // without drive limits the value wraps at the native type range instead
    setCurrentValue(steppedValue(currentValue(), step, rangeLow, rangeHigh, overflow));

    // an explicitly set waveform counts as well, element by element
    for(int i = 0; i < m_waveOverride.size(); i++) {
        m_waveOverride[i] = steppedValue(m_waveOverride.at(i), step, rangeLow, rangeHigh, overflow);
    }

    updateAlarmState();
    needsPublish = true;
}

bool InternalChannel::advance(int elapsedMs)
{
    if(mode != Counter) return false;
    bool changed = false;
    m_elapsedMs += elapsedMs;
    while(m_elapsedMs >= periodMs) {
        m_elapsedMs -= periodMs;
        tick();
        changed = true;
    }
    return changed;
}

// alarm evaluation like an EPICS record, status codes follow epicsAlarm.h
void InternalChannel::alarmState(double checkValue, short *severity, short *status) const
{
    if(hihi.defined && checkValue >= hihi.value)      { *severity = MAJOR_ALARM; *status = 3; }
    else if(lolo.defined && checkValue <= lolo.value) { *severity = MAJOR_ALARM; *status = 5; }
    else if(high.defined && checkValue >= high.value) { *severity = MINOR_ALARM; *status = 4; }
    else if(low.defined && checkValue <= low.value)   { *severity = MINOR_ALARM; *status = 6; }
    else                                              { *severity = NO_ALARM; *status = 0; }
}

// severities as in alarmdefs.h
static QStringList severityStrings()
{
    return QStringList() << "NO_ALARM" << "MINOR" << "MAJOR" << "INVALID" << "NOTCONNECTED";
}

// alarm status names in the order of epicsAlarm.h
static QStringList statusStrings()
{
    return QStringList() << "NO_ALARM" << "READ" << "WRITE" << "HIHI" << "HIGH" << "LOLO"
                         << "LOW" << "STATE" << "COS" << "COMM" << "TIMEOUT" << "HWLIMIT"
                         << "CALC" << "SCAN" << "LINK" << "SOFT" << "BAD_SUB" << "UDF"
                         << "DISABLE" << "SIMM" << "READ_ACCESS" << "WRITE_ACCESS";
}

static bool severityFromWrite(qint32 idata, const QString &sdata, short *code)
{
    int index = severityStrings().indexOf(sdata.trimmed().toUpper());
    if(index == -1) {
        if(idata < 0) return false;
        index = idata;
    }
    if(index == 4 || index == NOTCONNECTED) *code = NOTCONNECTED;
    else                                    *code = (short) qBound((int) NO_ALARM, index, (int) INVALID_ALARM);
    return true;
}

static bool statusFromWrite(qint32 idata, const QString &sdata, short *code)
{
    int index = statusStrings().indexOf(sdata.trimmed().toUpper());
    if(index == -1) {
        if(idata < 0) return false;
        index = idata;
    }
    *code = (short) qBound(0, index, statusStrings().size() - 1);
    return true;
}

static void writeStringsToDataB(knobData *kData, const QStringList &items);

void InternalChannel::updateAlarmState()
{
    alarmState(currentValue(), &severity, &status);
}

void InternalChannel::setFieldValue(Field field, double rdata, qint32 idata, const QString &sdata)
{
    short code;
    switch(field) {
    case FieldVal:
        setValue(rdata, idata, sdata);
        return;
    // a written severity/status stays until the next value change re-evaluates the alarms
    case FieldSevr:
        if(severityFromWrite(idata, sdata, &code)) severity = code;
        break;
    case FieldStat:
        if(statusFromWrite(idata, sdata, &code)) status = code;
        break;
    // alarm limits re-evaluate the alarm state immediately
    case FieldLow:  low.set(rdata); updateAlarmState(); break;
    case FieldLolo: lolo.set(rdata); updateAlarmState(); break;
    case FieldHigh: high.set(rdata); updateAlarmState(); break;
    case FieldHihi: hihi.set(rdata); updateAlarmState(); break;
    case FieldDrvl: drvl.set(rdata); break;
    case FieldDrvh: drvh.set(rdata); break;
    case FieldPrec: precision = (short) ((rdata != 0.0) ? rdata : idata); break;
    case FieldEgu:  units = sdata; break;
    case FieldNord: nord = qBound(0, (idata != 0) ? (int) idata : (int) rdata, nelm); break;
    case FieldNelm: // read only
        return;
    }
    needsPublish = true;
}

QVariant InternalChannel::fieldVariant(Field field) const
{
    switch(field) {
    case FieldSevr: return (int) severity;
    case FieldStat: return (int) status;
    case FieldLow:  return low.value;
    case FieldLolo: return lolo.value;
    case FieldHigh: return high.value;
    case FieldHihi: return hihi.value;
    case FieldDrvl: return drvl.value;
    case FieldDrvh: return drvh.value;
    case FieldPrec: return (int) precision;
    case FieldEgu:  return units;
    case FieldNelm: return nelm;
    case FieldNord: return nord;
    default:        return QVariant();
    }
}

void InternalChannel::fillKnobDataField(knobData *kData, Field field) const
{
    if(field == FieldVal) {
        fillKnobData(kData);
        return;
    }

    kData->edata.connected = true;
    kData->edata.accessR = true;
    kData->edata.accessW = (field != FieldNelm);
    kData->edata.severity = 0;
    kData->edata.status = 0;
    kData->edata.nelm = 1;
    kData->edata.valueCount = 1;
    kData->edata.precision = precision;
    qstrncpy(kData->edata.units, units.toLatin1().constData(), caqtdm_string_t_length);

    switch(field) {

    case FieldSevr: {
        long index = (severity == NOTCONNECTED)
                         ? 4 : qBound((int) NO_ALARM, (int) severity, (int) INVALID_ALARM);
        kData->edata.fieldtype = caENUM;
        kData->edata.rvalue = (double) index;
        kData->edata.ivalue = index;
        kData->edata.enumCount = severityStrings().size();
        writeStringsToDataB(kData, severityStrings());
        break;
    }

    case FieldStat: {
        long index = qBound(0, (int) status, statusStrings().size() - 1);
        kData->edata.fieldtype = caENUM;
        kData->edata.rvalue = (double) index;
        kData->edata.ivalue = index;
        kData->edata.enumCount = statusStrings().size();
        writeStringsToDataB(kData, statusStrings());
        break;
    }

    case FieldEgu:
        kData->edata.fieldtype = caSTRING;
        kData->edata.rvalue = 0.0;
        kData->edata.ivalue = 0;
        writeStringsToDataB(kData, QStringList() << units);
        break;

    case FieldNelm:
    case FieldNord: {
        long count = (field == FieldNelm) ? nelm : nord;
        kData->edata.fieldtype = caLONG;
        kData->edata.rvalue = (double) count;
        kData->edata.ivalue = count;
        kData->edata.precision = 0;
        break;
    }

    default: {
        double fieldValue = 0.0;
        switch(field) {
        case FieldLow:  fieldValue = low.value; break;
        case FieldLolo: fieldValue = lolo.value; break;
        case FieldHigh: fieldValue = high.value; break;
        case FieldHihi: fieldValue = hihi.value; break;
        case FieldDrvl: fieldValue = drvl.value; break;
        case FieldDrvh: fieldValue = drvh.value; break;
        case FieldPrec: fieldValue = (double) precision; break;
        default: break;
        }
        kData->edata.fieldtype = caDOUBLE;
        kData->edata.rvalue = fieldValue;
        kData->edata.ivalue = (long) fieldValue;
        break;
    }
    }
}

void InternalChannel::setValue(double rdata, qint32 idata, const QString &sdata)
{
    switch(fieldtype) {
    case caSTRING:
        // a generator channel is positioned by index (idata); with a regex the
        // written string is processed like the macro modification; otherwise
        // the text is taken as written
        if(m_combinations > 0) {
            setCurrentValue((double) idata);
        } else if(!regexPattern.isEmpty()) {
            QString processed = sdata;
            processed.replace(QRegularExpression(regexPattern), m_regexReplace);
            text = processed;
        } else {
            text = sdata;
        }
        break;
    case caENUM: {
        int index = enums.indexOf(sdata);
        setCurrentValue((index >= 0) ? (double) index : (double) idata);
        break;
    }
    case caINT:
    case caLONG:
    case caCHAR:
        setCurrentValue((double) idata);
        break;
    default:
        setCurrentValue(rdata);
        break;
    }
    m_waveOverride.clear();
    textArray.clear();
    m_elapsedMs = 0;
    updateAlarmState();
    needsPublish = true;
}

void InternalChannel::setWave(const QVector<double> &values)
{
    m_waveOverride = values;
    // like an EPICS waveform record a write updates NORD, capped at NELM
    nord = qMin(values.size(), nelm);
    if(!values.isEmpty()) setCurrentValue(values.at(0));
    updateAlarmState();
    needsPublish = true;
}

double InternalChannel::elementValue(int i) const
{
    if(i < m_waveOverride.size()) return m_waveOverride.at(i);
    // deterministic ramp: element i is the current value plus i steps
    return currentValue() + i * step;
}

// allocates (or reuses) the dataB buffer of kData with the requested size
static void allocateDataB(knobData *kData, int size)
{
    if(size != kData->edata.dataSize) {
        if(kData->edata.dataB != (void *) Q_NULLPTR) free(kData->edata.dataB);
        kData->edata.dataB = (void *) malloc((size_t) size);
        kData->edata.dataSize = size;
    }
}

// writes a list of strings into dataB, separated by '\033' as the epics3 plugin does
static void writeStringsToDataB(knobData *kData, const QStringList &items)
{
    QByteArray joined = items.join(QChar('\033')).toLatin1();
    allocateDataB(kData, joined.size() + 1);
    char *ptr = (char *) kData->edata.dataB;
    memcpy(ptr, joined.constData(), (size_t) joined.size());
    ptr[joined.size()] = '\0';
}

void InternalChannel::fillKnobData(knobData *kData) const
{
    kData->edata.fieldtype = fieldtype;
    kData->edata.connected = (severity != NOTCONNECTED);
    kData->edata.accessR = true;
    kData->edata.accessW = true;
    kData->edata.precision = precision;
    kData->edata.nelm = nelm;
    qstrncpy(kData->edata.units, units.toLatin1().constData(), caqtdm_string_t_length);

    // drive limits become display and control limits
    if(drvl.defined) {
        kData->edata.lower_disp_limit = drvl.value;
        kData->edata.lower_ctrl_limit = drvl.value;
    }
    if(drvh.defined) {
        kData->edata.upper_disp_limit = drvh.value;
        kData->edata.upper_ctrl_limit = drvh.value;
    }

    // alarm limits and the resulting severity/status for the current value
    if(low.defined)  kData->edata.lower_warning_limit = low.value;
    if(lolo.defined) kData->edata.lower_alarm_limit = lolo.value;
    if(high.defined) kData->edata.upper_warning_limit = high.value;
    if(hihi.defined) kData->edata.upper_alarm_limit = hihi.value;
    kData->edata.severity = severity;
    kData->edata.status = status;

    switch(fieldtype) {

    case caENUM: {
        long index = (long) native.enumValue;
        if(!enums.isEmpty()) index = qBound((long) 0, index, (long) enums.size() - 1);
        kData->edata.rvalue = (double) index;
        kData->edata.ivalue = index;
        kData->edata.valueCount = 1;
        kData->edata.enumCount = enums.size();
        writeStringsToDataB(kData, enums);
        break;
    }

    case caSTRING: {
        kData->edata.rvalue = 0.0;
        kData->edata.ivalue = 0;
        kData->edata.valueCount = nord;
        QStringList items;
        for(int i = 0; i < nord; i++) {
            // explicit array content first, then regex generated strings
            // (consecutive matches for arrays), otherwise the fixed text
            if(!textArray.isEmpty())    items << textArray.value(i);
            else if(m_combinations > 0) items << generatedString((qint64) native.doubleValue + i);
            else                        items << text;
        }
        writeStringsToDataB(kData, items);
        break;
    }

    case caCHAR: {
        kData->edata.rvalue = (double) native.charValue;
        kData->edata.ivalue = (long) native.charValue;
        kData->edata.valueCount = nord;
        allocateDataB(kData, nord + 1);
        char *ptr = (char *) kData->edata.dataB;
        for(int i = 0; i < nord; i++) ptr[i] = (char) (quint8) ((qint64) elementValue(i));
        ptr[nord] = '\0';
        break;
    }

    case caINT: {
        kData->edata.rvalue = (double) native.int16Value;
        kData->edata.ivalue = (long) native.int16Value;
        kData->edata.valueCount = nord;
        if(nelm > 1) {
            allocateDataB(kData, nord * (int) sizeof(qint16));
            qint16 *ptr = (qint16 *) kData->edata.dataB;
            for(int i = 0; i < nord; i++) ptr[i] = (qint16) (qint64) elementValue(i);
        }
        break;
    }

    case caLONG: {
        kData->edata.rvalue = (double) native.int32Value;
        kData->edata.ivalue = (long) native.int32Value;
        kData->edata.valueCount = nord;
        if(nelm > 1) {
            allocateDataB(kData, nord * (int) sizeof(qint32));
            qint32 *ptr = (qint32 *) kData->edata.dataB;
            for(int i = 0; i < nord; i++) ptr[i] = (qint32) (qint64) elementValue(i);
        }
        break;
    }

    case caFLOAT: {
        kData->edata.rvalue = (double) native.floatValue;
        kData->edata.ivalue = (long) native.floatValue;
        kData->edata.valueCount = nord;
        if(nelm > 1) {
            allocateDataB(kData, nord * (int) sizeof(float));
            float *ptr = (float *) kData->edata.dataB;
            for(int i = 0; i < nord; i++) ptr[i] = (float) elementValue(i);
        }
        break;
    }

    case caDOUBLE:
    default: {
        kData->edata.rvalue = native.doubleValue;
        kData->edata.ivalue = (long) native.doubleValue;
        kData->edata.valueCount = nord;
        if(nelm > 1) {
            allocateDataB(kData, nord * (int) sizeof(double));
            double *ptr = (double *) kData->edata.dataB;
            for(int i = 0; i < nord; i++) ptr[i] = elementValue(i);
        }
        break;
    }
    }
}
