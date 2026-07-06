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
#include "tst_internal_plugin.h"
#include "qtdefinitions.h"

#include <QMutex>

#include <string.h>

void TestInternalPlugin::initTestCase()
{
    m_plugin = Q_NULLPTR;
    m_mutexKnobData = Q_NULLPTR;
}

void TestInternalPlugin::init()
{
    m_plugin = new InternalPlugin();
    m_mutexKnobData = new MutexKnobData();
    m_widgets.clear();
    m_indexes.clear();

    QCOMPARE(m_plugin->initCommunicationLayer(m_mutexKnobData, Q_NULLPTR, QMap<QString, QString>()), (int) true);
}

void TestInternalPlugin::cleanupTestCase()
{
}

void TestInternalPlugin::cleanup()
{
    foreach(int index, m_indexes) {
        knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
        if(kData != (knobData *) Q_NULLPTR) {
            m_plugin->pvFreeAllocatedData(kData);
            delete (QMutex *) kData->mutex;
            kData->mutex = (void *) Q_NULLPTR;
        }
    }
    m_indexes.clear();

    delete m_plugin;
    delete m_mutexKnobData;
    qDeleteAll(m_widgets);
    m_widgets.clear();
}

// registers a monitor for the given pv like caqtdm_lib does; the configuration
// is carried by the channelConfigJSON property of the defining widget (as the
// genSoftPV widget does) and read by the plugin through kData->dispW
int TestInternalPlugin::createMonitor(const QString &pv, const QString &configJSON)
{
    QWidget *widget = new QWidget();
    if(!configJSON.isEmpty()) widget->setProperty("channelConfigJSON", configJSON);
    m_widgets.append(widget);

    int index = m_mutexKnobData->GetMutexKnobDataIndex();

    knobData kData;
    memset(&kData, 0, sizeof(knobData));
    kData.index = index;
    kData.soft = 0;
    kData.thisW = (void *) widget;
    kData.dispW = (void *) widget;
    kData.mutex = (void *) new QMutex();
    qstrncpy(kData.pv, pv.toLatin1().constData(), MAXPVLEN - 1);
    qstrncpy(kData.pluginName, "internal", caqtdm_string_t_length);
    m_mutexKnobData->SetMutexKnobData(index, kData);

    knobData *kPtr = m_mutexKnobData->GetMutexKnobDataPtr(index);
    m_plugin->pvAddMonitor(index, kPtr, 0, 0);
    m_indexes.append(index);
    return index;
}

// executes one base interval of the publish timer without a running event loop
void TestInternalPlugin::pumpTimerOnce()
{
    QMetaObject::invokeMethod(m_plugin, "updateChannels", Qt::DirectConnection);
}

void TestInternalPlugin::pluginNameIsInternal()
{
    QCOMPARE(m_plugin->pluginName(), QString("internal"));
}

void TestInternalPlugin::addMonitorPublishesInitialValue()
{
    int index = createMonitor("READY", R"({"type":"double","val":4.5,"units":"V"})");

    pumpTimerOnce();

    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.connected, (int) true);
    QCOMPARE(kData->edata.fieldtype, (short) caDOUBLE);
    QCOMPARE(kData->edata.rvalue, 4.5);
    QCOMPARE(QString(kData->edata.units), QString("V"));
    QVERIFY(kData->edata.monitorCount > 0);
}

void TestInternalPlugin::configFromWidgetPropertyWorks()
{
    // full configuration far beyond MAXPVLEN, impossible as a channel name
    QString config = R"({"type":"double","mode":"counter","val":0,"step":0.5,"period":200,
                         "drvl":0,"drvh":100,"loop":true,
                         "low":20,"lolo":10,"high":80,"hihi":90,
                         "units":"V","prec":2})";
    QVERIFY(config.length() > 120);

    int index = createMonitor("RAMP", config);
    pumpTimerOnce();

    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.fieldtype, (short) caDOUBLE);
    QCOMPARE(kData->edata.lower_disp_limit, 0.0);
    QCOMPARE(kData->edata.upper_disp_limit, 100.0);
    QCOMPARE(kData->edata.lower_warning_limit, 20.0);
    QCOMPARE(kData->edata.lower_alarm_limit, 10.0);
    QCOMPARE(kData->edata.upper_warning_limit, 80.0);
    QCOMPARE(kData->edata.upper_alarm_limit, 90.0);
    QCOMPARE(QString(kData->edata.units), QString("V"));
    QCOMPARE(kData->edata.precision, (short) 2);

    // the counter starts below lolo: major alarm right away
    QCOMPARE(kData->edata.severity, (short) 2);

    InternalChannel *channel = m_plugin->channel("RAMP");
    QVERIFY(channel != Q_NULLPTR);
    QCOMPARE(channel->isConfigured(), true);
    QCOMPARE(channel->mode, InternalChannel::Counter);
}

void TestInternalPlugin::filterSuffixIsIgnored()
{
    // an epics filter suffix in the channel name is no plugin configuration:
    // it only gets stripped from the channel key
    int index = createMonitor(R"(FILT.{"dbnd":{"abs":0.5}})", R"({"type":"long","val":7})");
    pumpTimerOnce();

    QVERIFY(m_plugin->channel("FILT") != Q_NULLPTR);
    QCOMPARE(m_plugin->channel("FILT")->currentValue(), 7.0);
    QCOMPARE(m_plugin->channel(R"(FILT.{"dbnd":{"abs":0.5}})") == Q_NULLPTR, true);

    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.ivalue, 7L);
    QCOMPARE(kData->edata.fieldtype, (short) caLONG);
}

void TestInternalPlugin::channelsAreSharedByBaseName()
{
    int first = createMonitor("SHARED", R"({"type":"long","val":11})");
    int second = createMonitor("SHARED"); // no configuration: attaches to the existing channel

    pumpTimerOnce();

    knobData *kFirst = m_mutexKnobData->GetMutexKnobDataPtr(first);
    knobData *kSecond = m_mutexKnobData->GetMutexKnobDataPtr(second);
    QCOMPARE(kFirst->edata.ivalue, 11L);
    QCOMPARE(kSecond->edata.ivalue, 11L);
    QCOMPARE(kSecond->edata.fieldtype, (short) caLONG);

    // both monitors point to the very same channel object
    QCOMPARE(m_plugin->channel("SHARED") != Q_NULLPTR, true);
    QCOMPARE(m_plugin->channel("SHARED")->currentValue(), 11.0);
}

void TestInternalPlugin::invalidConfigFallsBackToDefaults()
{
    int index = createMonitor("BROKEN", R"({"type":"nonsense"})");

    pumpTimerOnce();

    // the channel still connects, with default double/constant 0
    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.connected, (int) true);
    QCOMPARE(kData->edata.fieldtype, (short) caDOUBLE);
    QCOMPARE(kData->edata.rvalue, 0.0);

    InternalChannel *channel = m_plugin->channel("BROKEN");
    QVERIFY(channel != Q_NULLPTR);
    QCOMPARE(channel->isConfigured(), false);
}

void TestInternalPlugin::counterAdvancesWithTimerTicks()
{
    // period 100 ms equals the base interval: every pump is one counter step
    int index = createMonitor("COUNT", R"({"type":"long","mode":"counter","val":0,"step":1,"period":100})");

    pumpTimerOnce(); // publishes and executes the first step
    pumpTimerOnce();
    pumpTimerOnce();

    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.ivalue, 3L);
}

void TestInternalPlugin::writeThroughPluginWorks()
{
    int index = createMonitor("SETPOINT", R"({"type":"double","val":1.0})");
    pumpTimerOnce();

    char pv[MAXPVLEN];
    char errmess[SMALL_STRING_LENGTH];
    errmess[0] = '\0';
    qstrncpy(pv, "SETPOINT", MAXPVLEN);

    // writing publishes immediately, without waiting for the timer
    QCOMPARE(m_plugin->pvSetValue(pv, 2.5, 0, (char *) "", (char *) "tst", errmess, 0), (int) true);
    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.rvalue, 2.5);

    // writing to an unknown channel fails
    qstrncpy(pv, "DOES_NOT_EXIST", MAXPVLEN);
    QCOMPARE(m_plugin->pvSetValue(pv, 1.0, 0, (char *) "", (char *) "tst", errmess, 0), (int) false);

    // waveform write path
    int waveIndex = createMonitor("WAVE", R"({"type":"double","nelm":3})");
    pumpTimerOnce();
    double waveData[3] = {5.0, 6.0, 7.0};
    qstrncpy(pv, "WAVE", MAXPVLEN);
    QCOMPARE(m_plugin->pvSetWave(pv, (float *) Q_NULLPTR, waveData, (int16_t *) Q_NULLPTR,
                                 (int32_t *) Q_NULLPTR, (char *) Q_NULLPTR, 3, (char *) "tst", errmess),
             (int) true);
    knobData *kWave = m_mutexKnobData->GetMutexKnobDataPtr(waveIndex);
    double *values = (double *) kWave->edata.dataB;
    QVERIFY(values != (double *) Q_NULLPTR);
    QCOMPARE(values[0], 5.0);
    QCOMPARE(values[2], 7.0);
}

void TestInternalPlugin::channelDeletedWhenUnreferenced()
{
    int first = createMonitor("TEMP", R"({"type":"long","mode":"counter","val":5,"period":100})");
    int second = createMonitor("TEMP");

    // one of two references removed: the channel stays
    m_plugin->pvClearMonitor(m_mutexKnobData->GetMutexKnobDataPtr(first));
    QVERIFY(m_plugin->channel("TEMP") != Q_NULLPTR);

    // reference count reaches zero: the channel is deleted
    m_plugin->pvClearMonitor(m_mutexKnobData->GetMutexKnobDataPtr(second));
    QVERIFY(m_plugin->channel("TEMP") == Q_NULLPTR);

    // a new definition starts over from its configuration
    createMonitor("TEMP", R"({"type":"long","mode":"counter","val":5,"period":100})");
    QVERIFY(m_plugin->channel("TEMP") != Q_NULLPTR);
    QCOMPARE(m_plugin->channel("TEMP")->currentValue(), 5.0);
}

void TestInternalPlugin::persistentChannelKeepsRunning()
{
    int index = createMonitor("PCOUNT", R"({"type":"long","mode":"counter","val":0,"step":1,"period":100,"persistent":true})");
    pumpTimerOnce(); // publishes and counts to 1

    // removing the last reference keeps a persistent channel alive
    m_plugin->pvClearMonitor(m_mutexKnobData->GetMutexKnobDataPtr(index));
    QVERIFY(m_plugin->channel("PCOUNT") != Q_NULLPTR);

    // and it keeps counting without any monitor
    pumpTimerOnce();
    pumpTimerOnce();
    QCOMPARE(m_plugin->channel("PCOUNT")->currentValue(), 3.0);

    // a display re-attaching (no configuration) continues with the live value
    int again = createMonitor("PCOUNT");
    pumpTimerOnce();
    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(again);
    QCOMPARE(kData->edata.ivalue, 4L);
}
