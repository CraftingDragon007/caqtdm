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
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */
#include <QDebug>
#include <QThread>
#include "demo_plugin.h"
// You need to modify this file to add a plugin-specific logging category
#include "caQtDM_Plugins_global.h"

// as defined in knobDefines.h
//caType {caSTRING	= 0, caINT = 1, caFLOAT = 2, caENUM = 3, caCHAR = 4, caLONG = 5, caDOUBLE = 6};

// this demo plugin just gives you an idea how to use a plugin; for more details you should take a look
// at the epics3 plugin

// This needs to be done ONLY once per Plugin.
Q_LOGGING_CATEGORY(demoLog, "caqtdm.plugins.demo")

// gives the plugin name back
QString DemoPlugin::pluginName()
{
    return "demo";
}

// constructor
DemoPlugin::DemoPlugin()
{
    // Logging this way, there is no need to manually specify information about where this log is from etc.
    qCDebug(demoLog) << "Demo: Create";
    timer = timerValues = Q_NULLPTR;
}

DemoPlugin::~DemoPlugin()
{
    if (timer) {
        timer->deleteLater();
    }
    if (timerValues) {
        timerValues->deleteLater();
    }
}

// in this demo we update our interface here; normally you should update in from your controlsystem
// take a look how monitors are treated in the epics3 plugin
void DemoPlugin::updateInterface()
{
    double newValue = 0.0;

    QMutexLocker locker(&mutex);

    // go through our devices
    foreach(int index, listOfIndexes) {
        knobData* kData = mutexknobdataP->GetMutexKnobDataPtr(index);
        if((kData != (knobData *) Q_NULLPTR) && (kData->index != -1)) {
            QString key = kData->pv;

            // find this pv in our internal double values list (assume for now we are only treating doubles)
            // and increment its value
            QMap<QString, double>::iterator i = listOfDoubles.find(key);
            while (i !=listOfDoubles.end() && i.key() == key) {
                newValue = i.value();
                break;
            }
            // update some data
            kData->edata.rvalue = newValue;
            kData->edata.fieldtype = caDOUBLE;
            kData->edata.connected = true;
            kData->edata.accessR = kData->edata.accessW = true;
            kData->edata.monitorCount++;
            mutexknobdataP->SetMutexKnobData(kData->index, *kData);
            mutexknobdataP->SetMutexKnobDataReceived(kData);
        }
    }
}

// in this demo we update our values here
void DemoPlugin::updateValues()
{
    QMutexLocker locker(&mutex);
#ifndef HARDWORK
    QMap<QString, double>::iterator i;
    for (i = listOfDoubles.begin(); i != listOfDoubles.end(); ++i) i.value()++;
#else
    QtConcurrent::run(this, &DemoPlugin::updateHardwork);
#endif
}

#ifdef HARDWORK
void  DemoPlugin::updateHardwork()
{
    qCDebug(demoLog) << "hardwork";
    QMap<QString, double>::iterator i;
    for (i = listOfDoubles.begin(); i != listOfDoubles.end(); ++i) i.value()++;
}
#endif

// initialize our communicationlayer with everything you need
int DemoPlugin::initCommunicationLayer(MutexKnobData *data, MessageWindow *messageWindow, QMap<QString, QString> options)
{
    qCDebug(demoLog) << "InitCommunicationLayer with options" << options;

    mutexknobdataP = data;
    messagewindowP = messageWindow;

    initValue = 0.0;

    // we want to update our internal doubles every second
    timerValues = new QTimer();
    connect(timerValues, SIGNAL(timeout()), this, SLOT(updateValues()));
    timerValues->start(1000);

    // we want to update the interface every 2 seconds
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(updateInterface()));
    timer->start(2000);

    return true;
}

// caQtDM_Lib will call this routine for defining a monitor
int DemoPlugin::pvAddMonitor(int index, knobData *kData, int rate, int skip) {
    Q_UNUSED(index);
    Q_UNUSED(rate);
    Q_UNUSED(skip);
    QMutexLocker locker(&mutex);
    QString key = kData->pv;

    qCDebug(demoLog) << "pvAddMonitor" << kData->pv << kData->index;
    double value = initValue;
    initValue += 10;

    // append device index to our internal list
    listOfIndexes.append(kData->index);

    // initial values into the doubles list
    if(!listOfDoubles.contains(key)) listOfDoubles.insert(key, value);

    return true;
}

// caQtDM_Lib will call this routine for getting rid of a monitor
int DemoPlugin::pvClearMonitor(knobData *kData) {
    QMutexLocker locker(&mutex);

    qCDebug(demoLog) << "pvClearMonitor" << kData->pv << kData->index;
    QString key = kData->pv;
    if(!listOfDoubles.contains(key)) listOfDoubles.remove(key);
    listOfIndexes.removeAll(kData->index);

    return true;
}

int DemoPlugin::pvFreeAllocatedData(knobData *kData)
{
    qCDebug(demoLog) << "DemoPlugin:pvFreeAllocatedData";
    if (kData->edata.info != (void *) Q_NULLPTR) {
        free(kData->edata.info);
        kData->edata.info = (void*) Q_NULLPTR;
    }
    if(kData->edata.dataB != (void*) Q_NULLPTR) {
        free(kData->edata.dataB);
        kData->edata.dataB = (void*) Q_NULLPTR;
    }

    return true;
}

// caQtDM_Lib will call this routine for setting data (see for more detail the epics3 plugin)
int DemoPlugin::pvSetValue(char *pv, double rdata, int32_t idata, char *sdata, char *object, char *errmess, int forceType) {
    Q_UNUSED(object);
    Q_UNUSED(errmess);
    Q_UNUSED(forceType);
    QMutexLocker locker(&mutex);
    qCDebug(demoLog) << "pvSetValue" << pv << rdata << idata << sdata;
    QString key = pv;
    if(listOfDoubles.contains(key)) listOfDoubles.insert(pv, rdata);
    return true;
}

// caQtDM_Lib will call this routine for setting waveforms data (see for more detail the epics3 plugin)
int DemoPlugin::pvSetWave(char *pv, float *fdata, double *ddata, int16_t *data16, int32_t *data32, char *sdata, int nelm, char *object, char *errmess) {
    Q_UNUSED(pv);
    Q_UNUSED(fdata);
    Q_UNUSED(ddata);
    Q_UNUSED(data16);
    Q_UNUSED(data32);
    Q_UNUSED(sdata);
    Q_UNUSED(nelm);
    Q_UNUSED(object);
    Q_UNUSED(errmess);
    QMutexLocker locker(&mutex);
    qCDebug(demoLog) << "pvSetWave";
    return true;
}

// caQtDM_Lib will call this routine for getting a description of the monitor
int DemoPlugin::pvGetTimeStamp(char *pv, char *timestamp) {
    Q_UNUSED(pv);
    Q_UNUSED(timestamp);
    qCDebug(demoLog) << "pvgetTimeStamp";
    qstrncpy(timestamp, "timestamp in epics format", TIMESTAMP_STRING_LENGTH);
    return true;
}

// caQtDM_Lib will call this routine for getting the timestamp for this monitor
int DemoPlugin::pvGetDescription(char *pv, char *description) {
    Q_UNUSED(pv);
    Q_UNUSED(description);
    qCDebug(demoLog) << "pvGetDescription";
    qstrncpy(description, "hello, I am a double", MAX_STRING_LENGTH);
    return true;
}

// next two routines are used to stop and restart the monitoring (used in case of tabWidgets in the display)
int DemoPlugin::pvClearEvent(void * ptr) {
    Q_UNUSED(ptr);
    qCDebug(demoLog) << "pvClearEvent";
    return true;
}

int DemoPlugin::pvAddEvent(void * ptr) {
    Q_UNUSED(ptr);
    qCDebug(demoLog) << "pvAddEvent";
    return true;
}

// next two routines are used to connect and disconnect monitors when the application gest suspended and reactivated
int DemoPlugin::pvReconnect(knobData *kData) {
    Q_UNUSED(kData);
    qCDebug(demoLog) << "pvReconnect";
    return true;
}

int DemoPlugin::pvDisconnect(knobData *kData) {
    Q_UNUSED(kData);
    qCDebug(demoLog) << "pvDisconnect";
    return true;
}

// flush any io is periodically called (1s timer) in order to flush the disconnection and reconnection
// used for pv's that will be hidden and shown in case of tabwidgets
int DemoPlugin::FlushIO() {
    qCDebug(demoLog) << "DemoPlugin:FlushIO";
    return true;
}

// termination (in case of epics3, this is used to destroy the context when the application gest deactivated
// otherwise probably no meaning; in this demo, we stop the simulation, however it will not be reactivated
// any more (you may do that through pvReconnect)
int DemoPlugin::TerminateIO() {
    qCDebug(demoLog) << "DemoPlugin:TerminateIO";
    timerValues->stop();
    timer->stop();
    return true;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#else
    Q_EXPORT_PLUGIN2(DemoPlugin, DemoPlugin)
#endif

