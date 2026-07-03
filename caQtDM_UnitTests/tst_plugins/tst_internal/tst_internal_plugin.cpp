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
    m_widget = Q_NULLPTR;
}

void TestInternalPlugin::init()
{
    m_plugin = new InternalPlugin();
    m_mutexKnobData = new MutexKnobData();
    m_widget = new QWidget();
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
    delete m_widget;
}

// registers a monitor for the given pv (may carry a JSON part) like caqtdm_lib does
int TestInternalPlugin::createMonitor(const QString &pv)
{
    int index = m_mutexKnobData->GetMutexKnobDataIndex();

    knobData kData;
    memset(&kData, 0, sizeof(knobData));
    kData.index = index;
    kData.soft = 0;
    kData.thisW = (void *) m_widget;
    kData.dispW = (void *) m_widget;
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
    int index = createMonitor(R"(READY.{"type":"double","val":4.5,"units":"V"})");

    pumpTimerOnce();

    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.connected, (int) true);
    QCOMPARE(kData->edata.fieldtype, (short) caDOUBLE);
    QCOMPARE(kData->edata.rvalue, 4.5);
    QCOMPARE(QString(kData->edata.units), QString("V"));
    QVERIFY(kData->edata.monitorCount > 0);
}

void TestInternalPlugin::channelsAreSharedByBaseName()
{
    int first = createMonitor(R"(SHARED.{"type":"long","val":11})");
    int second = createMonitor("SHARED"); // no JSON: attaches to the existing channel

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

void TestInternalPlugin::invalidJsonFallsBackToDefaults()
{
    int index = createMonitor(R"(BROKEN.{"type":"nonsense"})");

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
    int index = createMonitor(R"(COUNT.{"type":"long","mode":"counter","val":0,"step":1,"period":100})");

    pumpTimerOnce(); // publishes init and executes the first step
    pumpTimerOnce();
    pumpTimerOnce();

    knobData *kData = m_mutexKnobData->GetMutexKnobDataPtr(index);
    QCOMPARE(kData->edata.ivalue, 3L);
}

void TestInternalPlugin::writeThroughPluginWorks()
{
    int index = createMonitor(R"(SETPOINT.{"type":"double","val":1.0})");
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
    int waveIndex = createMonitor(R"(WAVE.{"type":"double","nelm":3})");
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
