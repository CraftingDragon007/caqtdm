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
#include "opcua_plugin.h"
#include <QDebug>
#include <QOpcUaMultiDimensionalArray>
#include <QSettings>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include "alarmdefs.h"
#include "fileFunctions.h"
#include "opcua_core.h"
#include "searchfile.h"
#include <memory>

OPCUAPlugin::OPCUAPlugin()
{
    VERBOSELOG("Create");
    QLoggingCategory::setFilterRules("qt.opcua.plugins.open62541*=false");
    m_mutexKnobDataP = Q_NULLPTR;
    m_messageWindowP = Q_NULLPTR;
}

QString OPCUAPlugin::pluginName()
{
    return "opcua";
}

int OPCUAPlugin::initCommunicationLayer(MutexKnobData *data,
                                        MessageWindow *messageWindow,
                                        QMap<QString, QString> options)
{
    VERBOSELOG("Initialized with num options: " << options.size());

    m_mutexKnobDataP = data;
    m_messageWindowP = messageWindow;

    QStringList opcua_database_files;

    QString url = (QString) qgetenv("CAQTM_URL_DISPLAY_PATH");
    QString database_file = (QString) qgetenv("CAQTDM_OPCUA_DATABASE");

    opcua_database_files.append(database_file.split(","));

    if (!options.value("OPCUA_DATABASE", "").isEmpty())
        opcua_database_files.append(options.value("OPCUA_DATABASE"));

    fileFunctions fileFunction;
    foreach (QString opcua_database_file, opcua_database_files) {
        if (!url.isEmpty()) {
            fileFunction.checkFileAndDownload(opcua_database_file, url);
        }
        searchFile *s = new searchFile(opcua_database_file);
        QString fileNameFound = s->findFile();

        delete s;

        if (!fileNameFound.isEmpty()) {
            QFile file(fileNameFound);
            if (file.open(QIODevice::ReadOnly)) {
                QString msg = "opcua translation found: ";
                msg.append(fileNameFound);
                if (m_messageWindowP != Q_NULLPTR)
                    m_messageWindowP->postMsgEvent(QtDebugMsg, msg.toUtf8().data());

                QTextStream in(&file);

                while (!in.atEnd()) {
                    QString line = in.readLine();
                    if (!line.trimmed().startsWith("#")) {
                        int equalIndex = line.indexOf(
                            "="); // Since Node Id's have '=' in them we have to make sure to only split at the first equal sign.
                        if (equalIndex > 0) {
                            QString key = line.left(equalIndex).trimmed();
                            QString val = line.mid(equalIndex + 1).trimmed();
                            m_translationMap.insert(key, val);
                        }
                    }
                }
                file.close();
            }
        }
    }

    if (m_messageWindowP) {
        for (const QString &file : opcua_database_files) {
            QString msg = "OPCUA: Loaded database file: " + file;
            m_messageWindowP->postMsgEvent(QtDebugMsg, msg.toUtf8().data());
        }
    }

    if (m_messageWindowP) {
        for (auto it = m_translationMap.constBegin(); it != m_translationMap.constEnd(); ++it) {
            QString msg = QString("OPCUA: %1 => %2").arg(it.key(), it.value());
            m_messageWindowP->postMsgEvent(QtDebugMsg, msg.toUtf8().data());
        }
    }

    if (m_messageWindowP) {
        QString msg = "OPCUA: Make sure the channels dont have semicolons in them. If neccessary, "
                      "use CAQTDM_OPCUA_DATABASE";
        m_messageWindowP->postMsgEvent(QtWarningMsg, msg.toUtf8().data());
    }

    return true;
}

int OPCUAPlugin::pvAddMonitor(int index, knobData *kData, int rate, int skip)
{
    Q_UNUSED(rate);
    Q_UNUSED(skip);

    QString rawPV = QString::fromUtf8(kData->pv);
    if (rawPV.endsWith(".FTVL")) {
        QString regularPV = rawPV;
        regularPV.remove(rawPV.length() - 5, 5);
        if (!m_epicsWaveformAttributePVs.contains(regularPV)) {
            m_epicsWaveformAttributePVs[regularPV] = {-1, -1};
        }
        m_epicsWaveformAttributePVs[regularPV].FTVL_index = index;
        return true;
    } else if (rawPV.endsWith(".NELM")) {
        QString regularPV = rawPV;
        regularPV.remove(rawPV.length() - 5, 5);
        if (!m_epicsWaveformAttributePVs.contains(regularPV)) {
            m_epicsWaveformAttributePVs[regularPV] = {-1, -1};
        }
        m_epicsWaveformAttributePVs[regularPV].NELM_index = index;
        return true;
    }

    QString endpoint, nodeId;
    if (!resolveConnectionString(kData->pv, endpoint, nodeId)) {
        if (m_messageWindowP) {
            QString msg = "Invalid OPCUA PV format. Expected <endpoint>/ns=...; got: "
                          + QString::fromUtf8(kData->pv);
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    };

    int samplingIntervalMs = getUpdateIntervalFromKnobData(kData);
    SubscriptionSettings pendingSubscription = {nodeId, samplingIntervalMs};

    m_channelCache.insert(nodeId, index);

    OpcUaCore *core;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_cores.contains(endpoint)) {
            m_cores[endpoint] = new OpcUaCore();
            m_connectionState[endpoint] = ConnectionState::NotConnected;

            core = m_cores[endpoint];
            QObject::connect(core,
                             &OpcUaCore::valueRead,
                             this,
                             [=](const QString &nodeId, const QVariant &value) {
                                 auto range = m_channelCache.equal_range(nodeId);
                                 for (auto it = range.first; it != range.second; ++it) {
                                     int idx = it.value();
                                     knobData kData = m_mutexKnobDataP->GetMutexKnobData(idx);

                                     updateKnobDataFromVariant(kData, value);
                                     if (!kData.edata.connected) {
                                         m_mutexKnobDataP->SetMutexKnobDataConnected(idx, true);
                                     }

                                     m_mutexKnobDataP->SetMutexKnobDataReceived(&kData);
                                 }
                             });

            QObject::connect(core,
                             &OpcUaCore::accessLevelRead,
                             this,
                             [=](const QString &nodeId,
                                 const bool &readAccess,
                                 const bool &writeAccess) {
                                 auto range = m_channelCache.equal_range(nodeId);
                                 for (auto it = range.first; it != range.second; ++it) {
                                     int idx = it.value();
                                     knobData kData = m_mutexKnobDataP->GetMutexKnobData(idx);

                                     updateKnobDataWithAccessLevel(kData, readAccess, writeAccess);
                                     updateEpicsWaveformAttributePVs(rawPV, kData);
                                     m_mutexKnobDataP->SetMutexKnobDataConnected(idx, true);

                                     m_mutexKnobDataP->SetMutexKnobDataReceived(&kData);
                                 }
                             });

            QObject::connect(core, &OpcUaCore::disconnected, this, [=]() {
                m_connectionState[endpoint] = ConnectionState::NotConnected;
                for (int idx : m_knobDataIndicesForEndpoint[endpoint]) {
                    knobData kData = m_mutexKnobDataP->GetMutexKnobData(idx);

                    m_mutexKnobDataP->SetMutexKnobDataConnected(idx, false);

                    updateEpicsWaveformAttributePVs(rawPV, kData);
                }

                if (m_knobDataIndicesForEndpoint[endpoint].length() > 0) {
                    if (m_messageWindowP) {
                        QString err = QString("OPCUA: Connection failed for %1").arg(endpoint);
                        m_messageWindowP->postMsgEvent(QtCriticalMsg, err.toUtf8().data());
                    }
                }
            });

            QObject::connect(core, &OpcUaCore::connected, this, [=]() {
                m_connectionState[endpoint] = ConnectionState::Connected;
                if (m_messageWindowP) {
                    QString info = QString("OPCUA: Connected to %1").arg(endpoint);
                    m_messageWindowP->postMsgEvent(QtInfoMsg, info.toUtf8().data());
                }

                // Subscribe to all pending nodeIds
                for (const auto &pendingSubscription : m_pendingSubscriptions[endpoint]) {
                    core->subscribeToNode(pendingSubscription);
                }
                m_pendingSubscriptions[endpoint].clear();
            });

            QObject::connect(core,
                             &OpcUaCore::attributeGotError,
                             this,
                             [=](const QString &nodeId, const QString &errorMsg) {
                                 auto range = m_channelCache.equal_range(nodeId);
                                 for (auto it = range.first; it != range.second; ++it) {
                                     int idx = it.value();
                                     knobData kData = m_mutexKnobDataP->GetMutexKnobData(idx);

                                     VERBOSELOG("nodeId: " << nodeId << " got error: " << errorMsg);

                                     kData.edata.severity = INVALID_ALARM;
                                     kData.edata.status = 1; // READ_ALARM
                                     m_mutexKnobDataP->SetMutexKnobData(kData.index, kData);
                                     m_mutexKnobDataP->SetMutexKnobDataReceived(&kData);
                                 }
                             });
        } else {
            core = m_cores[endpoint];
        }
    }

    m_knobDataIndicesForEndpoint[endpoint].push_back(index);

    QMutexLocker lock(&m_mutex);
    ConnectionState state = m_connectionState[endpoint];

    if (state == ConnectionState::Connected) {
        core->subscribeToNode(pendingSubscription);
    } else {
        // Store subscription to do later
        m_pendingSubscriptions[endpoint].append(pendingSubscription);

        if (state == ConnectionState::NotConnected) {
            m_connectionState[endpoint] = ConnectionState::Connecting;

            // Start connection
            core->connectOpc(endpoint);
        }
        // Else the core is already connecting, meaning once it finishes it will subscribe to the stored subscription
    }
    return true;
}

int OPCUAPlugin::pvClearMonitor(knobData *kData)
{
    QMutexLocker lock(&m_mutex);

    QString rawPV = QString::fromUtf8(kData->pv);
    if (rawPV.endsWith(".FTVL")) {
        QString regularPV = rawPV;
        regularPV.remove(rawPV.length() - 5, 5);
        if (!m_epicsWaveformAttributePVs.contains(regularPV)) {
            m_epicsWaveformAttributePVs[regularPV] = {-1, -1};
        }
        m_epicsWaveformAttributePVs[regularPV].FTVL_index = -1;
        return true;
    } else if (rawPV.endsWith(".NELM")) {
        QString regularPV = rawPV;
        regularPV.remove(rawPV.length() - 5, 5);
        if (!m_epicsWaveformAttributePVs.contains(regularPV)) {
            m_epicsWaveformAttributePVs[regularPV] = {-1, -1};
        }
        m_epicsWaveformAttributePVs[regularPV].NELM_index = -1;
        return true;
    }

    int index = kData->index;
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData->pv, endpoint, nodeId)) {
        if (m_messageWindowP) {
            QString msg = "Invalid OPCUA PV format. Expected <endpoint>/ns=...; got: "
                          + QString::fromUtf8(kData->pv);
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    };

    if (nodeId.isEmpty()) {
        if (m_messageWindowP) {
            QString msg = QString("OPCUA: No nodeId found for index %1").arg(index);
            m_messageWindowP->postMsgEvent(QtDebugMsg, msg.toUtf8().data());
        }
        return false;
    }

    m_knobDataIndicesForEndpoint[endpoint].removeAll(index);
    m_channelCache.remove(nodeId, index);

    // Find which endpoint this node belongs to
    for (auto it = m_cores.begin(); it != m_cores.end(); ++it) {
        auto core = it.value();
        if (core && core->hasSubscription(nodeId)) {
            core->unsubscribeFromNode(nodeId);
            break;
        }
    }

    return true;
}

int OPCUAPlugin::pvFreeAllocatedData(knobData *kData)
{
    QMutexLocker locker(static_cast<QMutex *>(kData->mutex));
    if (kData->edata.dataB) {
        free(kData->edata.dataB);
        kData->edata.dataB = Q_NULLPTR;
    }
    return true;
}

int OPCUAPlugin::pvSetValue(
    char *pv, double rdata, int32_t idata, char *sdata, char *object, char *errmess, int forceType)
{
    Q_UNUSED(object);
    Q_UNUSED(forceType);

    QMutexLocker locker(&m_mutex);

    QString endpoint, nodeId;
    if (!resolveConnectionString(pv, endpoint, nodeId)) {
        return false;
    }

    if (!m_cores.contains(endpoint)) {
        if (m_messageWindowP) {
            QString msg = "Tried writing to pv that's not connected correctly: " + endpoint;
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    }

    auto &core = m_cores[endpoint];
    return core->writeValue(nodeId, rdata, idata, sdata, errmess);
}

int OPCUAPlugin::pvSetWave(char *pv,
                           float *fdata,
                           double *ddata,
                           int16_t *data16,
                           int32_t *data32,
                           char *sdata,
                           int nelm,
                           char *object,
                           char *errmess)
{
    Q_UNUSED(object);

    QMutexLocker locker(&m_mutex);

    QString endpoint, nodeId;
    if (!resolveConnectionString(pv, endpoint, nodeId)) {
        return false;
    }

    if (!m_cores.contains(endpoint)) {
        if (m_messageWindowP) {
            QString msg = "Tried writing to pv that's not connected correctly: " + endpoint;
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    }

    auto &core = m_cores[endpoint];
    return core->writeValues(nodeId, fdata, ddata, data16, data32, sdata, nelm, errmess);
}

int OPCUAPlugin::pvGetTimeStamp(char *pv, char *timestamp)
{
    Q_UNUSED(pv);
    QString rawPV = QString::fromUtf8(pv);
    if (rawPV.endsWith(".FTVL") || rawPV.endsWith(".NELM")) {
        rawPV.remove(rawPV.length() - 5, 5);
    }

    QString endpoint, nodeId;
    if (!resolveConnectionString(rawPV.toUtf8().data(), endpoint, nodeId)) {
        return false;
    }

    if (m_cores.contains(endpoint)) {
        auto &core = m_cores[endpoint];
        QString nodeTimestamp = core->getTimestamp(nodeId);
        qstrncpy(timestamp, nodeTimestamp.toUtf8().data(), MAX_STRING_LENGTH);
    } else {
        if (m_messageWindowP) {
            QString msg = "[pvGetTimeStamp]: endpoint not configured: " + endpoint;
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    }

    return true;
}

int OPCUAPlugin::pvGetDescription(char *pv, char *description)
{
    QString rawPV = QString::fromUtf8(pv);
    if (rawPV.endsWith(".FTVL") || rawPV.endsWith(".NELM")) {
        qstrcpy(description, "I'm just here for compatibility reasons :)");
        return true;
    }

    QString endpoint, nodeId;
    if (!resolveConnectionString(pv, endpoint, nodeId)) {
        return false;
    }

    if (m_cores.contains(endpoint)) {
        auto &core = m_cores[endpoint];
        QString nodeDescription = core->getDescription(nodeId);
        qstrncpy(description, nodeDescription.toUtf8().data(), MAX_STRING_LENGTH);
    } else {
        if (m_messageWindowP) {
            QString msg = "[pvGetDescription]: endpoint not configured: " + endpoint;
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    }

    return true;
}

int OPCUAPlugin::pvClearEvent(void *ptr)
{
    knobData *kData = static_cast<knobData *>(ptr);
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData->pv, endpoint, nodeId)) {
        return false;
    };

    QMutexLocker locker(&m_mutex);
    if (m_cores.contains(endpoint)) {
        m_cores[endpoint]->disableMonitoringForNode(nodeId);
        VERBOSELOG("Paused monitoring for" << nodeId);
    }

    return true;
}

int OPCUAPlugin::pvAddEvent(void *ptr)
{
    knobData *kData = static_cast<knobData *>(ptr);
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData->pv, endpoint, nodeId)) {
        return false;
    };

    int samplingIntervalMs = getUpdateIntervalFromKnobData(kData);
    SubscriptionSettings pendingSubscription = {nodeId, samplingIntervalMs};

    QMutexLocker locker(&m_mutex);
    if (m_cores.contains(endpoint)) {
        m_cores[endpoint]->subscribeToNode(pendingSubscription);
        VERBOSELOG("Resumed monitoring for" << nodeId);
    }

    return true;
}

int OPCUAPlugin::pvReconnect(knobData *kData)
{
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData->pv, endpoint, nodeId)) {
        if (m_messageWindowP) {
            QString msg = "Invalid OPCUA PV format. Expected <endpoint>/ns=...; got: "
                          + QString::fromUtf8(kData->pv);
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return false;
    };

    int samplingIntervalMs = getUpdateIntervalFromKnobData(kData);
    SubscriptionSettings pendingSubscription = {nodeId, samplingIntervalMs};

    QMutexLocker lock(&m_mutex);
    if (!m_cores.contains(endpoint))
        return false;

    auto core = m_cores[endpoint];
    core->disconnectOpc();
    m_connectionState[endpoint] = ConnectionState::NotConnected;
    m_pendingSubscriptions[endpoint].append(pendingSubscription);

    core->connectOpc(endpoint);
    return true;
}

int OPCUAPlugin::pvDisconnect(knobData *kData)
{
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData->pv, endpoint, nodeId)) {
        return false;
    };

    QMutexLocker lock(&m_mutex);
    if (m_cores.contains(endpoint)) {
        m_cores[endpoint]->disconnectOpc();
        m_connectionState[endpoint] = ConnectionState::NotConnected;
        VERBOSELOG("Disconnected from" << endpoint);
    }
    return true;
}

int OPCUAPlugin::FlushIO()
{
    return true;
}

int OPCUAPlugin::TerminateIO()
{
    QMutexLocker locker(&m_mutex);

    for (auto &core : m_cores) {
        if (core) {
            core->clearAllSubscriptions();
            core->disconnectOpc();
            core->deleteLater();
        }
    }

    m_cores.clear();
    m_connectionState.clear();
    m_pendingSubscriptions.clear();
    m_channelCache.clear();

    return true;
}

bool OPCUAPlugin::resolveConnectionString(char *pv, QString &endpoint, QString &nodeId)
{
    QString plainKey = QString::fromLatin1(pv);
    QString logicalKey = plainKey.remove("opcua://");
    QString fullConnection = m_translationMap.value(plainKey,
                                                    m_translationMap.value(logicalKey, ""));
    if (fullConnection.isEmpty()) {
        fullConnection = plainKey;
    }

    QString raw = fullConnection.remove("opcua://");

    int splitPos = raw.lastIndexOf("/ns=");
    if (splitPos < 0) {
        splitPos = raw.lastIndexOf("/i=");
        if (splitPos < 0) {
            VERBOSELOG("Invalid connection string: " << fullConnection);
            return false;
        }
    }

    endpoint = raw.left(splitPos);
    nodeId = raw.mid(splitPos + 1).trimmed();
    return true;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QT_VARIANT_TYPE(value) value.typeId()
#else
#define QT_VARIANT_TYPE(value) value.type()
#endif

caType OPCUAPlugin::generateCaTypeFromVariant(const QVariant &value, bool &isArray, bool &isMatrix)
{
    isArray = false;
    QVariant valueToCheck = value;
    isMatrix = valueToCheck.canConvert<QOpcUaMultiDimensionalArray>();

    if ((valueToCheck.canConvert<QVariantList>() || isMatrix)
        && !valueToCheck.canConvert<QString>()) {
        if (isMatrix) {
            valueToCheck = valueToCheck.value<QOpcUaMultiDimensionalArray>().valueArray();
        }
        QList<QVariant> list = valueToCheck.toList();
        if (list.length() > 0) {
            valueToCheck = list.constFirst();
            isArray = true;
        }
    }

    switch (QT_VARIANT_TYPE(valueToCheck)) {
    case QMetaType::Double:
        return caDOUBLE;
    case QMetaType::Float:
        return caFLOAT;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Long:
    case QMetaType::ULong:
        return caLONG;
    case QMetaType::Short:
    case QMetaType::Bool:
        return caINT;
    case QMetaType::QString:
        return caSTRING;
    default:
        return caDOUBLE;
    }
}

void OPCUAPlugin::updateKnobDataFromVariantSingle(knobData &kData,
                                                  const QVariant &value,
                                                  const caType &detectedType)
{
    kData.edata.valueCount = kData.edata.nelm = 1;
    switch (detectedType) {
    case caDOUBLE:
        kData.edata.rvalue = value.toDouble();
        kData.edata.ivalue = kData.edata.rvalue;
        kData.edata.precision = 8;
        break;
    case caFLOAT:
        kData.edata.rvalue = value.toFloat();
        kData.edata.ivalue = kData.edata.rvalue;
        kData.edata.precision = 4;
        break;
    case caLONG:
        kData.edata.ivalue = value.toInt();
        kData.edata.rvalue = kData.edata.ivalue;
        kData.edata.precision = 0;
        break;
    case caINT:
        kData.edata.ivalue = static_cast<int16_t>(value.toInt());
        kData.edata.rvalue = kData.edata.ivalue;
        kData.edata.precision = 0;
        break;
    case caSTRING:
        if (kData.edata.dataB && kData.edata.dataSize != (value.toString().length() + 1)) {
            free(kData.edata.dataB);
            kData.edata.dataB = Q_NULLPTR;
        }
        if (!kData.edata.dataB) {
            kData.edata.dataSize = value.toString().length() + 1;
            kData.edata.dataB = malloc(static_cast<size_t>(kData.edata.dataSize));
        }
        memcpy(kData.edata.dataB,
               value.toString().toUtf8().data(),
               static_cast<size_t>(kData.edata.dataSize));
        break;
    default:
        kData.edata.rvalue = 0.0;
        kData.edata.ivalue = 0;
        break;
    }
}

void OPCUAPlugin::updateEpicsWaveformAttributePVs(QString rawPV, const knobData &referenceKnobData)
{
    if (rawPV.endsWith(".FTVL") || rawPV.endsWith(".NELM")) {
        rawPV.remove(rawPV.length() - 5, 5);
    }
    if (m_epicsWaveformAttributePVs.contains(rawPV)) {
        EpicsWaveformAttributePVs indices = m_epicsWaveformAttributePVs[rawPV];
        if (indices.FTVL_index != -1) {
            knobData FTVL_kdata = m_mutexKnobDataP->GetMutexKnobData(indices.FTVL_index);
            FTVL_kdata.edata.connected = referenceKnobData.edata.connected;
            FTVL_kdata.edata.ivalue = referenceKnobData.edata.fieldtype;
            FTVL_kdata.edata.rvalue = FTVL_kdata.edata.ivalue;
            FTVL_kdata.edata.fieldtype = caINT;
            m_mutexKnobDataP->SetMutexKnobData(indices.FTVL_index, FTVL_kdata);
        }
        if (indices.NELM_index != -1) {
            knobData NELM_kdata = m_mutexKnobDataP->GetMutexKnobData(indices.NELM_index);
            NELM_kdata.edata.connected = referenceKnobData.edata.connected;
            NELM_kdata.edata.ivalue = referenceKnobData.edata.valueCount;
            NELM_kdata.edata.rvalue = NELM_kdata.edata.ivalue;
            NELM_kdata.edata.fieldtype = caINT;
            m_mutexKnobDataP->SetMutexKnobData(indices.NELM_index, NELM_kdata);
        }
    }
}

void OPCUAPlugin::updateKnobDataFromVariantArray(knobData &kData,
                                                 const QVariant &value,
                                                 const caType &detectedType)
{
    QList<QVariant> list = value.toList();
    int num_values = list.count();
    kData.edata.valueCount = kData.edata.nelm = num_values;

    QString rawPV = QString::fromUtf8(kData.pv);
    updateEpicsWaveformAttributePVs(rawPV, kData);

    switch (detectedType) {
    case caDOUBLE: {
        if (kData.edata.dataB && kData.edata.dataSize != num_values * sizeof(double)) {
            free(kData.edata.dataB);
            kData.edata.dataB = Q_NULLPTR;
        }

        if (!kData.edata.dataB) {
            kData.edata.dataSize = num_values * sizeof(double);
            kData.edata.dataB = malloc(static_cast<size_t>(kData.edata.dataSize));
        }

        double *intBuffer = static_cast<double *>(kData.edata.dataB);
        for (int i = 0; i < num_values; ++i) {
            intBuffer[i] = list[i].toDouble();
        }

        kData.edata.precision = 8;
        break;
    }
    case caFLOAT: {
        if (kData.edata.dataB && kData.edata.dataSize != num_values * sizeof(float)) {
            free(kData.edata.dataB);
            kData.edata.dataB = Q_NULLPTR;
        }

        if (!kData.edata.dataB) {
            kData.edata.dataSize = num_values * sizeof(float);
            kData.edata.dataB = malloc(static_cast<size_t>(kData.edata.dataSize));
        }

        float *intBuffer = static_cast<float *>(kData.edata.dataB);
        for (int i = 0; i < num_values; ++i) {
            intBuffer[i] = list[i].toFloat();
        }

        kData.edata.precision = 4;
        break;
    }
    case caLONG: {
        if (kData.edata.dataB && kData.edata.dataSize != num_values * sizeof(int32_t)) {
            free(kData.edata.dataB);
            kData.edata.dataB = Q_NULLPTR;
        }

        if (!kData.edata.dataB) {
            kData.edata.dataSize = num_values * sizeof(int32_t);
            kData.edata.dataB = malloc(static_cast<size_t>(kData.edata.dataSize));
        }

        int32_t *intBuffer = static_cast<int32_t *>(kData.edata.dataB);
        for (int i = 0; i < num_values; ++i) {
            intBuffer[i] = list[i].toInt();
        }

        kData.edata.precision = 0;
        break;
    }
    case caINT: {
        if (kData.edata.dataB && kData.edata.dataSize != num_values * sizeof(int16_t)) {
            free(kData.edata.dataB);
            kData.edata.dataB = Q_NULLPTR;
        }

        if (!kData.edata.dataB) {
            kData.edata.dataSize = num_values * sizeof(int16_t);
            kData.edata.dataB = malloc(static_cast<size_t>(kData.edata.dataSize));
        }

        int16_t *intBuffer = static_cast<int16_t *>(kData.edata.dataB);
        for (int i = 0; i < num_values; ++i) {
            intBuffer[i] = static_cast<int16_t>(list[i].toInt());
        }

        kData.edata.precision = 0;
        break;
    }
    case caSTRING: {
        QVariant firstValue = list.constFirst();
        if (kData.edata.dataB && kData.edata.dataSize != (firstValue.toString().length() + 1)) {
            free(kData.edata.dataB);
            kData.edata.dataB = Q_NULLPTR;
        }
        if (!kData.edata.dataB) {
            kData.edata.dataSize = firstValue.toString().length() + 1;
            kData.edata.dataB = malloc(static_cast<size_t>(kData.edata.dataSize));
        }
        memcpy(kData.edata.dataB,
               firstValue.toString().toUtf8().data(),
               static_cast<size_t>(kData.edata.dataSize));
        break;
    }
    default:
        kData.edata.rvalue = 0.0;
        kData.edata.ivalue = 0;
        break;
    }
}

void OPCUAPlugin::updateKnobDataFromVariant(knobData &kData, QVariant value)
{
    QMutexLocker locker(static_cast<QMutex *>(kData.mutex));
    bool isArray = false;
    bool isMatrix = false;
    caType detectedType = generateCaTypeFromVariant(value, isArray, isMatrix);
    if (isMatrix) {
        value = value.value<QOpcUaMultiDimensionalArray>().valueArray();
    }
    kData.edata.fieldtype = detectedType;
    kData.edata.connected = 1;
    kData.edata.monitorCount++;

    if (isArray) {
        updateKnobDataFromVariantArray(kData, value, detectedType);
    } else {
        updateKnobDataFromVariantSingle(kData, value, detectedType);
    }
}

void OPCUAPlugin::updateKnobDataWithAccessLevel(knobData &kData,
                                                const bool &accessR,
                                                const bool &accessW)
{
    QMutexLocker locker(static_cast<QMutex *>(kData.mutex));
    kData.edata.accessR = accessR;
    kData.edata.accessW = accessW;
}

int OPCUAPlugin::getUpdateIntervalFromKnobData(knobData *kData)
{
    if (!kData) {
        if (m_messageWindowP) {
            QString msg = "Received invalid KnobData for unknown PV";
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
        return 1;
    }
    int updateRateHz = kData->edata.repRate;
    if (updateRateHz == 0) {
        updateRateHz = 1;
        if (m_messageWindowP) {
            QString msg = "Invalid OPCUA PV refresh rate of 0 Hz. Defaulted to 1 Hz. PV: "
                          + QString::fromUtf8(kData->pv);
            m_messageWindowP->postMsgEvent(QtCriticalMsg, msg.toUtf8().data());
        }
    }
    int samplingIntervalMs = 1000 / updateRateHz;
    return samplingIntervalMs;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#else
Q_EXPORT_PLUGIN2(DemoPlugin, DemoPlugin)
#endif
