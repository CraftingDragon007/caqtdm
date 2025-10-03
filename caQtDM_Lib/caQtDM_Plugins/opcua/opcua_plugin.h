/*
 *  This file is part of the caQtDM Framework, it was developed in colaboration with
 *  the University of Lucerene (HSLU) as a Economy Project and the Paul Scherrer Institut.
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
 *  Copyright (c) 2025
 *
 *  Authors:
 *    Hrvat Leo
 *    Joel Müller
 */
#ifndef OPCUA_PLUGIN_H
#define OPCUA_PLUGIN_H

#pragma once

#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QtGlobal>
#include "controlsinterface.h"
#include "opcua_core.h"

//#define HARDWORK

#ifdef HARDWORK
#include <QtConcurrentRun>
#endif

// Holds the mutexKnobData indices for channels carrying EPICS waveform attributes corresponding to a channel (.NELM, .FTVL)
typedef struct
{
    int NELM_index;
    int FTVL_index;
} EpicsWaveformAttributePVs;

class Q_DECL_EXPORT OPCUAPlugin : public QObject, ControlsInterface
{
    Q_OBJECT
    Q_INTERFACES(ControlsInterface)
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID "ch.psi.caqtdm.Plugin.ControlsInterface/1.0.opcua")
#endif

public:
    OPCUAPlugin();

    QString pluginName();

    int initCommunicationLayer(MutexKnobData *data,
                               MessageWindow *messageWindow,
                               QMap<QString, QString> options);
    int pvAddMonitor(int index, knobData *kData, int rate, int skip);
    int pvClearMonitor(knobData *kData);
    int pvFreeAllocatedData(knobData *kData);
    int pvSetValue(char *pv,
                   double rdata,
                   int32_t idata,
                   char *sdata,
                   char *object,
                   char *errmess,
                   int forceType);
    int pvSetWave(char *pv,
                  float *fdata,
                  double *ddata,
                  int16_t *data16,
                  int32_t *data32,
                  char *sdata,
                  int nelm,
                  char *object,
                  char *errmess);
    int pvGetTimeStamp(char *pv, char *timestamp);
    int pvGetDescription(char *pv, char *description);
    int pvClearEvent(void *ptr);
    int pvAddEvent(void *ptr);
    int pvReconnect(knobData *kData);
    int pvDisconnect(knobData *kData);
    int FlushIO();
    int TerminateIO();

private:
    enum class ConnectionState { NotConnected, Connecting, Connected };

    QMutex m_mutex;
    MutexKnobData *m_mutexKnobDataP;
    MessageWindow *m_messageWindowP;
    QMultiMap<QString, int> m_channelCache;
    QMap<QString, OpcUaCore *> m_cores;
    QMap<QString, QMetaObject::Connection> m_statusCallbackConnections;
    QMap<QString, QList<int>> m_knobDataIndicesForEndpoint;
    QMap<QString, ConnectionState> m_connectionState;
    QMap<QString, QList<SubscriptionSettings>> m_pendingSubscriptions;
    QMap<QString, EpicsWaveformAttributePVs> m_epicsWaveformAttributePVs;

    QMap<QString, QString> m_translationMap;

    caType generateCaTypeFromVariant(const QVariant &value, bool &isArray);
    int getUpdateIntervalFromKnobData(knobData *kData);
    bool resolveConnectionString(char *pv, QString &endpoint, QString &nodeId);
    void updateKnobDataFromVariantSingle(knobData &kData,
                                         const QVariant &value,
                                         const caType &detectedType);
    void updateKnobDataFromVariantArray(knobData &kData,
                                        const QVariant &value,
                                        const caType &detectedType);
    void updateKnobDataFromVariant(knobData &kData, const QVariant &value);
    void updateKnobDataWithAccessLevel(knobData &kData, const bool &accessR, const bool &accessW);
    void updateEpicsWaveformAttributePVs(QString rawPV, const knobData &referenceKnobData);

#ifdef HARDWORK
    void updateHardwork();
#endif
};

#endif // OPCUA_PLUGIN_H
