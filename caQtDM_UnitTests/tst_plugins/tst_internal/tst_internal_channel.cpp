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
#include "tst_internal_channel.h"

#include <string.h>

// fresh zero initialized knobData whose dataB is freed by freeKnobData below
static knobData makeKnobData()
{
    knobData kData;
    memset(&kData, 0, sizeof(knobData));
    return kData;
}

static void freeKnobData(knobData *kData)
{
    if(kData->edata.dataB != (void *) Q_NULLPTR) {
        free(kData->edata.dataB);
        kData->edata.dataB = (void *) Q_NULLPTR;
        kData->edata.dataSize = 0;
    }
}

void TestInternalChannel::baseNameAndJsonPartWork()
{
    QCOMPARE(InternalChannel::baseName("RAMP"), QString("RAMP"));
    QCOMPARE(InternalChannel::jsonPart("RAMP"), QString());

    QCOMPARE(InternalChannel::baseName(R"(RAMP.{"type":"double"})"), QString("RAMP"));
    QCOMPARE(InternalChannel::jsonPart(R"(RAMP.{"type":"double"})"), QString(R"({"type":"double"})"));

    // a record-like dot without JSON brace stays part of the name
    QCOMPARE(InternalChannel::baseName("DEVICE.VAL"), QString("DEVICE.VAL"));
}

void TestInternalChannel::configureParsesAllFields()
{
    InternalChannel channel;
    QString error;
    bool ok = channel.configure(R"({"type":"float","mode":"counter","init":5,"step":2.5,
                                    "period":200,"min":1,"max":9,"loop":false,"nelm":4,
                                    "units":"mA","prec":3,"value":"hello"})", &error);
    QVERIFY2(ok, qPrintable(error));
    QVERIFY(channel.isConfigured());
    QCOMPARE(channel.fieldtype, (short) caFLOAT);
    QCOMPARE(channel.mode, InternalChannel::Counter);
    QCOMPARE(channel.init, 5.0);
    QCOMPARE(channel.step, 2.5);
    QCOMPARE(channel.periodMs, 200);
    QCOMPARE(channel.minimum, 1.0);
    QCOMPARE(channel.maximum, 9.0);
    QCOMPARE(channel.hasMinimum, true);
    QCOMPARE(channel.hasMaximum, true);
    QCOMPARE(channel.loop, false);
    QCOMPARE(channel.nelm, 4);
    QCOMPARE(channel.units, QString("mA"));
    QCOMPARE(channel.precision, (short) 3);
    QCOMPARE(channel.text, QString("hello"));
    QCOMPARE(channel.value, 5.0);

    InternalChannel enumChannel;
    ok = enumChannel.configure(R"({"type":"enum","enums":["A","B","C"]})", &error);
    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(enumChannel.fieldtype, (short) caENUM);
    QCOMPARE(enumChannel.enums, QStringList() << "A" << "B" << "C");
}

void TestInternalChannel::configureUsesDefaults()
{
    InternalChannel channel;
    QString error;
    QVERIFY2(channel.configure("{}", &error), qPrintable(error));
    QCOMPARE(channel.fieldtype, (short) caDOUBLE);
    QCOMPARE(channel.mode, InternalChannel::Constant);
    QCOMPARE(channel.init, 0.0);
    QCOMPARE(channel.step, 1.0);
    QCOMPARE(channel.periodMs, 1000);
    QCOMPARE(channel.hasMinimum, false);
    QCOMPARE(channel.hasMaximum, false);
    QCOMPARE(channel.loop, true);
    QCOMPARE(channel.nelm, 1);
    QCOMPARE(channel.precision, (short) 2);

    // an enum without explicit states gets a default state list
    InternalChannel enumChannel;
    QVERIFY(enumChannel.configure(R"({"type":"enum"})", &error));
    QCOMPARE(enumChannel.enums, QStringList() << "OFF" << "ON");

    // integer types default to precision 0
    InternalChannel intChannel;
    QVERIFY(intChannel.configure(R"({"type":"int"})", &error));
    QCOMPARE(intChannel.precision, (short) 0);
}

void TestInternalChannel::configureRejectsInvalidInput()
{
    InternalChannel channel;
    QString error;

    QCOMPARE(channel.configure("{kaputt}", &error), false);
    QVERIFY(!error.isEmpty());
    QCOMPARE(channel.isConfigured(), false);

    QCOMPARE(channel.configure(R"({"type":"quaternion"})", &error), false);
    QCOMPARE(channel.isConfigured(), false);

    QCOMPARE(channel.configure(R"({"mode":"randomwalk"})", &error), false);
    QCOMPARE(channel.isConfigured(), false);

    QCOMPARE(channel.configure(R"([1,2,3])", &error), false);
    QCOMPARE(channel.isConfigured(), false);
}

void TestInternalChannel::counterTicksAndWraps()
{
    InternalChannel channel;
    QString error;
    QVERIFY2(channel.configure(R"({"mode":"counter","init":8,"step":1,"min":0,"max":10,"loop":true})", &error),
             qPrintable(error));

    channel.tick();
    QCOMPARE(channel.value, 9.0);
    channel.tick();
    QCOMPARE(channel.value, 10.0);
    channel.tick(); // beyond max -> wraps to min
    QCOMPARE(channel.value, 0.0);

    // without loop the counter saturates at max
    InternalChannel saturating;
    QVERIFY(saturating.configure(R"({"mode":"counter","init":9,"step":2,"min":0,"max":10,"loop":false})", &error));
    saturating.tick();
    QCOMPARE(saturating.value, 10.0);
    saturating.tick();
    QCOMPARE(saturating.value, 10.0);

    // negative step wraps at the lower limit
    InternalChannel backwards;
    QVERIFY(backwards.configure(R"({"mode":"counter","init":1,"step":-1,"min":0,"max":5,"loop":true})", &error));
    backwards.tick();
    QCOMPARE(backwards.value, 0.0);
    backwards.tick();
    QCOMPARE(backwards.value, 5.0);

    // an enum cycles through its states without explicit limits
    InternalChannel enumChannel;
    QVERIFY(enumChannel.configure(R"({"type":"enum","mode":"counter","enums":["A","B","C"]})", &error));
    QCOMPARE(enumChannel.value, 0.0);
    enumChannel.tick();
    enumChannel.tick();
    QCOMPARE(enumChannel.value, 2.0);
    enumChannel.tick();
    QCOMPARE(enumChannel.value, 0.0);

    // constant mode never changes the value
    InternalChannel constant;
    QVERIFY(constant.configure(R"({"init":7})", &error));
    constant.tick();
    QCOMPARE(constant.value, 7.0);
}

void TestInternalChannel::advanceRespectsPeriod()
{
    InternalChannel channel;
    QString error;
    QVERIFY2(channel.configure(R"({"mode":"counter","init":0,"step":1,"period":250})", &error),
             qPrintable(error));

    QCOMPARE(channel.advance(100), false);
    QCOMPARE(channel.advance(100), false);
    QCOMPARE(channel.advance(100), true); // 300 ms accumulated -> one tick
    QCOMPARE(channel.value, 1.0);

    // remaining 50 ms are kept, a big step executes several ticks
    QCOMPARE(channel.advance(700), true); // 750 ms -> three ticks
    QCOMPARE(channel.value, 4.0);

    // constant mode never reports a change
    InternalChannel constant;
    QVERIFY(constant.configure("{}", &error));
    QCOMPARE(constant.advance(10000), false);
}

void TestInternalChannel::regexGeneratorWorks()
{
    QString error;

    // character class with quantifier: STATE-00 .. STATE-99, wrapping around
    InternalChannel channel;
    QVERIFY2(channel.configure(R"({"type":"string","mode":"counter","regex":"STATE-[0-9]{2}"})", &error),
             qPrintable(error));
    QCOMPARE(channel.combinations(), Q_INT64_C(100));
    QCOMPARE(channel.generatedString(0), QString("STATE-00"));
    QCOMPARE(channel.generatedString(7), QString("STATE-07"));
    QCOMPARE(channel.generatedString(42), QString("STATE-42"));
    QCOMPARE(channel.generatedString(99), QString("STATE-99"));
    QCOMPARE(channel.generatedString(100), QString("STATE-00")); // wraps

    // the counter advances the generated string
    knobData kData = makeKnobData();
    channel.fillKnobData(&kData);
    QCOMPARE(QString((char *) kData.edata.dataB), QString("STATE-00"));
    channel.tick();
    channel.fillKnobData(&kData);
    QCOMPARE(QString((char *) kData.edata.dataB), QString("STATE-01"));
    freeKnobData(&kData);

    // starting index via init, wrap at the end of the combinations
    InternalChannel startAt;
    QVERIFY(startAt.configure(R"({"type":"string","mode":"counter","init":99,"regex":"STATE-[0-9]{2}"})", &error));
    QCOMPARE(startAt.generatedString((qint64) startAt.value), QString("STATE-99"));
    startAt.tick();
    QCOMPARE(startAt.generatedString((qint64) startAt.value), QString("STATE-00"));

    // alternation group and character ranges, last segment changes fastest
    InternalChannel combined;
    QVERIFY(combined.configure(R"({"type":"string","regex":"(ON|OFF)-[a-c]"})", &error));
    QCOMPARE(combined.combinations(), Q_INT64_C(6));
    QCOMPARE(combined.generatedString(0), QString("ON-a"));
    QCOMPARE(combined.generatedString(2), QString("ON-c"));
    QCOMPARE(combined.generatedString(3), QString("OFF-a"));
    QCOMPARE(combined.generatedString(5), QString("OFF-c"));

    // escaped characters stay literal
    InternalChannel escaped;
    QVERIFY(escaped.configure(R"({"type":"string","regex":"V\\[[0-1]\\]"})", &error));
    QCOMPARE(escaped.combinations(), Q_INT64_C(2));
    QCOMPARE(escaped.generatedString(1), QString("V[1]"));

    // writing positions a regex channel by index
    InternalChannel writable;
    QVERIFY(writable.configure(R"({"type":"string","regex":"MSG-[0-9]"})", &error));
    writable.setValue(0.0, 7, QString());
    QCOMPARE(writable.generatedString((qint64) writable.value), QString("MSG-7"));

    // invalid patterns are rejected
    InternalChannel broken;
    QCOMPARE(broken.configure(R"({"type":"string","regex":"[0-9"})", &error), false);
    QVERIFY(!error.isEmpty());
    QCOMPARE(broken.configure(R"({"type":"string","regex":"[9-0]"})", &error), false);
    QCOMPARE(broken.configure(R"({"type":"string","regex":"A{x}"})", &error), true); // quantifier only after class/group -> literal
}

void TestInternalChannel::fillKnobDataScalarTypesWork()
{
    QString error;

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"double","init":3.5,"min":-10,"max":10,"units":"V","prec":4})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.fieldtype, (short) caDOUBLE);
        QCOMPARE(kData.edata.rvalue, 3.5);
        QCOMPARE(kData.edata.ivalue, 3L);
        QCOMPARE(kData.edata.connected, (int) true);
        QCOMPARE(kData.edata.accessW, (int) true);
        QCOMPARE(kData.edata.precision, (short) 4);
        QCOMPARE(QString(kData.edata.units), QString("V"));
        QCOMPARE(kData.edata.lower_disp_limit, -10.0);
        QCOMPARE(kData.edata.upper_disp_limit, 10.0);
        QCOMPARE(kData.edata.lower_ctrl_limit, -10.0);
        QCOMPARE(kData.edata.upper_ctrl_limit, 10.0);
        QCOMPARE(kData.edata.valueCount, 1);
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"long","init":42})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.fieldtype, (short) caLONG);
        QCOMPARE(kData.edata.ivalue, 42L);
        QCOMPARE(kData.edata.rvalue, 42.0);
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"enum","init":1,"enums":["OFF","ON","ERROR"]})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.fieldtype, (short) caENUM);
        QCOMPARE(kData.edata.ivalue, 1L);
        QCOMPARE(kData.edata.enumCount, 3);
        QCOMPARE(QString((char *) kData.edata.dataB), QString("OFF\033ON\033ERROR"));
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"string","value":"hello world"})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.fieldtype, (short) caSTRING);
        QCOMPARE(QString((char *) kData.edata.dataB), QString("hello world"));
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"char","init":65})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.fieldtype, (short) caCHAR);
        QCOMPARE(kData.edata.ivalue, 65L);
        QCOMPARE(((char *) kData.edata.dataB)[0], 'A');
        freeKnobData(&kData);
    }
}

void TestInternalChannel::fillKnobDataArraysWork()
{
    QString error;

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"double","mode":"counter","init":10,"step":2,"nelm":5})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.valueCount, 5);
        QCOMPARE(kData.edata.nelm, 5);
        QCOMPARE(kData.edata.dataSize, (int) (5 * sizeof(double)));
        double *values = (double *) kData.edata.dataB;
        for(int i = 0; i < 5; i++) QCOMPARE(values[i], 10.0 + 2.0 * i); // deterministic ramp
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"int","init":1,"nelm":3})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.dataSize, (int) (3 * sizeof(int16_t)));
        int16_t *values = (int16_t *) kData.edata.dataB;
        QCOMPARE(values[0], (int16_t) 1);
        QCOMPARE(values[2], (int16_t) 3);
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"long","init":100,"nelm":3})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.dataSize, (int) (3 * sizeof(int32_t)));
        int32_t *values = (int32_t *) kData.edata.dataB;
        QCOMPARE(values[2], (int32_t) 102);
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"float","init":0.5,"step":0.5,"nelm":4})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        QCOMPARE(kData.edata.dataSize, (int) (4 * sizeof(float)));
        float *values = (float *) kData.edata.dataB;
        QCOMPARE(values[3], 2.0f);
        freeKnobData(&kData);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"char","init":65,"nelm":3})", &error));
        knobData kData = makeKnobData();
        channel.fillKnobData(&kData);
        char *values = (char *) kData.edata.dataB;
        QCOMPARE(values[0], 'A');
        QCOMPARE(values[1], 'B');
        QCOMPARE(values[2], 'C');
        freeKnobData(&kData);
    }
}

void TestInternalChannel::setValueWorks()
{
    QString error;

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"double","mode":"counter","init":0,"step":1})", &error));
        channel.setValue(12.5, 0, QString());
        QCOMPARE(channel.value, 12.5);
        channel.tick(); // the counter continues from the written value
        QCOMPARE(channel.value, 13.5);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"long"})", &error));
        channel.setValue(0.0, 77, QString());
        QCOMPARE(channel.value, 77.0);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"enum","enums":["OFF","ON","ERROR"]})", &error));
        channel.setValue(0.0, 0, "ERROR"); // writing the state name selects its index
        QCOMPARE(channel.value, 2.0);
        channel.setValue(0.0, 1, "unknown state");
        QCOMPARE(channel.value, 1.0);
    }

    {
        InternalChannel channel;
        QVERIFY(channel.configure(R"({"type":"string","value":"before"})", &error));
        channel.setValue(0.0, 0, "after");
        QCOMPARE(channel.text, QString("after"));
    }
}

void TestInternalChannel::setWaveWorks()
{
    QString error;
    InternalChannel channel;
    QVERIFY(channel.configure(R"({"type":"double","nelm":3,"init":0})", &error));

    QVector<double> wave;
    wave << 7.0 << 8.0 << 9.0;
    channel.setWave(wave);
    QCOMPARE(channel.value, 7.0);

    knobData kData = makeKnobData();
    channel.fillKnobData(&kData);
    double *values = (double *) kData.edata.dataB;
    QCOMPARE(values[0], 7.0);
    QCOMPARE(values[1], 8.0);
    QCOMPARE(values[2], 9.0);
    freeKnobData(&kData);
}
