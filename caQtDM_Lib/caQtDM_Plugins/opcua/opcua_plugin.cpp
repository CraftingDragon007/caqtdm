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
#include <QDebug>
#include <QThread>
#include "opcua_plugin.h"
#include "opcua_core.h"
#include <QSettings>
#include <memory>
#include "fileFunctions.h"
#include "searchfile.h"
#include <QtConcurrent/QtConcurrent>
#define qasc(x) x.toLatin1().constData()

extern "C"{
MutexKnobData* mutexKnobdataPtr;
MessageWindow *messageWindowPtr;
}

QString OPCUAPlugin::pluginName()
{
    return "opcua";
}

// constructor
OPCUAPlugin::OPCUAPlugin()
{
    qDebug() << "OPCUAPlugin: Create";
}

// initialize our communicationlayer with everything you need
int OPCUAPlugin::initCommunicationLayer(MutexKnobData *data, MessageWindow *messageWindow, QMap<QString, QString> options)
{
    qDebug() << "OPCUA Plugin: InitCommunicationLayer with options " << options;

    mutexknobdataP = data;
    messagewindowP = messageWindow;
    mutexKnobdataPtr = data;
    messageWindowPtr = messageWindow;
    optionsP=options;

    QStringList opcua_database_files;

    QString url = (QString) qgetenv("CAQTM_URL_DISPLAY_PATH");
    qDebug() << "URL: " << url;
    QString database_file = (QString) qgetenv("CAQTDM_OPCUA_DATABASE");

    opcua_database_files.append(database_file.split(","));

    if(optionsP.value("OPCUA_DATABASE", "").isEmpty())
        opcua_database_files.append(optionsP.value("OPCUA_DATABASE", ""));

    fileFunctions fileFunction;
    foreach(QString opcua_database_file, opcua_database_files){
        if(!url.isEmpty()){
            fileFunction.checkFileAndDownload(opcua_database_file, url);
        }
        searchFile *s = new searchFile(opcua_database_file);
        QString fileNameFound = s->findFile();

        delete s;

        if(!fileNameFound.isEmpty()){
            QFile file(fileNameFound);
            if(file.open(QIODevice::ReadOnly)){
                QString msg = "opcua translation found: ";
                msg.append(fileNameFound);
                if(messagewindowP != Q_NULLPTR) messagewindowP->postMsgEvent(QtDebugMsg, (char*)msg.toLatin1().constData());

                QTextStream in(&file);

                while(!in.atEnd()){
                    QString line = in.readLine();
                    if(!line.trimmed().startsWith("#")){
                        int equalIndex = line.indexOf("="); // Since Node Id's have '=' in them we have to make sure to only split at the first equal sign.
                        if(equalIndex > 0){
                        QString key = line.left(equalIndex).trimmed();
                        QString val = line.mid(equalIndex + 1).trimmed();
                        opcua_translation_map.insert(key, val);
                        }
                    }
                }
                file.close();
            }
        }

    }

    if (messageWindowPtr) {
        for (const QString &file : opcua_database_files) {
            QString msg = "OPCUA: Loaded database file: " + file;
            messageWindowPtr->postMsgEvent(QtDebugMsg, (char*)msg.toLatin1().constData());
        }
    }

    if (messageWindowPtr) {
        for (auto it = opcua_translation_map.constBegin(); it != opcua_translation_map.constEnd(); ++it) {
            QString msg = QString("OPCUA: %1 => %2").arg(it.key(), it.value());
            messageWindowPtr->postMsgEvent(QtDebugMsg, (char*)msg.toLatin1().constData());
        }
    }

    if(messageWindowPtr){
        QString msg= "Info: OPCUA Plugin has been loaded.";
        messageWindowPtr->postMsgEvent(QtInfoMsg,(char *) qasc(msg));
    }

    if (!m_core)
        m_core.reset(new opc::OpcUaCore());

    return true;
}

// caQtDM_Lib will call this routine for defining a monitor
int OPCUAPlugin::pvAddMonitor(int index, knobData *kData, int rate, int skip)
{
    Q_UNUSED(rate);
    Q_UNUSED(skip);

    QString key = QString::fromLatin1(kData->pv).trimmed();
    QString raw = opcua_translation_map.value(key).trimmed();

    if(raw.isEmpty()){
        raw=key;

//        if(messageWindowPtr){
//            QString msg = QString("OPCUA: No mapping found for key: %1").arg(key);
//            messageWindowPtr->postMsgEvent(QtCriticalMsg, (char*)msg.toLatin1().constData());
//        }

    }

//    int sep = raw.indexOf("::");
//    if (sep >= 0) raw = raw.mid(sep + 2);
//    if (raw.startsWith("opcua://", Qt::CaseInsensitive)) {
//        raw = raw.mid(QString("opcua://").length());
//    }

    int splitPos = raw.lastIndexOf("/ns=");
    if (splitPos < 0) {
        splitPos = raw.lastIndexOf("/i=");
        if (splitPos < 0) {
            if (messageWindowPtr) {
                QString msg = "Invalid OPCUA PV format. Expected <endpoint>/ns=...; got: " + raw;
                messageWindowPtr->postMsgEvent(QtCriticalMsg, (char*)msg.toLatin1().constData());
            }
            return false;
        }
    }

    QString endpoint = raw.left(splitPos);
    QString nodeId = raw.mid(splitPos + 1).trimmed();
    Channelcache.insert(nodeId, index);

    std::shared_ptr<opc::OpcUaCore> core;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_cores.contains(endpoint)) {
            m_cores[endpoint] = std::make_shared<opc::OpcUaCore>();
            m_connectionState[endpoint] = ConnectionState::NotConnected;

            auto core = m_cores[endpoint];
            QObject::connect(core.get(), &opc::OpcUaCore::valueRead, [=](const QString &nodeId, const QVariant &value) {
                auto range = Channelcache.equal_range(nodeId);
                for (auto it = range.first; it != range.second; ++it) {
                    int idx = it.value();
                    knobData kData = mutexKnobdataPtr->GetMutexKnobData(idx);

                    updateKnobDataFromVariant(kData, value); // Refactored into seperate Method to increase readability

                    mutexknobdataP->SetMutexKnobData(kData.index, kData);
                    mutexknobdataP->SetMutexKnobDataReceived(&kData);
                }
            });
        }
        core = m_cores[endpoint];
    }

    QMutexLocker lock(&m_mutex);
    ConnectionState state = m_connectionState[endpoint];

    if (state == ConnectionState::Connected) {
        core->subscribeToNode(nodeId);
    } else {
        // Store subscription to do later
        m_pendingSubscriptions[endpoint].append(nodeId);

        if (state == ConnectionState::NotConnected) {
            m_connectionState[endpoint] = ConnectionState::Connecting;

            // Start connection
            core->connectOpc(endpoint, [this, endpoint, core](bool success) {
                QMutexLocker lock(&m_mutex);
                if (!success) {
                    m_connectionState[endpoint] = ConnectionState::NotConnected;
                    if (messageWindowPtr) {
                        QString err = QString("OPCUA: Failed to connect to %1").arg(endpoint);
                        messageWindowPtr->postMsgEvent(QtCriticalMsg, (char*)err.toLatin1().constData());
                    }
                    return;
                }

                m_connectionState[endpoint] = ConnectionState::Connected;

                if (messageWindowPtr) {
                    QString info = QString("OPCUA: Connected to %1").arg(endpoint);
                    messageWindowPtr->postMsgEvent(QtInfoMsg, (char*)info.toLatin1().constData());
                }

                // Now subscribe to all pending nodeIds
                for (const QString& nodeId : m_pendingSubscriptions[endpoint]) {
                    core->subscribeToNode(nodeId);
                }
                m_pendingSubscriptions[endpoint].clear();
            });
        }
    }

    return true;
}


// caQtDM_Lib will call this routine for getting rid of a monitor
int OPCUAPlugin::pvClearMonitor(knobData *kData) {

    QMutexLocker lock(&m_mutex);

    int index = kData->index;
    QString nodeId = findNodeIdByIndex(index);

    if (nodeId.isEmpty()) {
        if (messageWindowPtr) {
            QString msg = QString("OPCUA: No nodeId found for index %1").arg(index);
            messageWindowPtr->postMsgEvent(QtDebugMsg, (char*)msg.toLatin1().constData());
        }
        return false;
    }

    // Remove from Channelcache
    Channelcache.remove(nodeId, index);

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
    QMutexLocker locker((QMutex *)kData->mutex);
    if (kData->edata.dataB) {
        free(kData->edata.dataB);
        kData->edata.dataB = nullptr;
    }
    return true;
}

// caQtDM_Lib will call this routine for setting data (see for more detail the epics3 plugin)
int OPCUAPlugin::pvSetValue(char *pv, double rdata, int32_t idata, char *sdata, char *object, char *errmess, int forceType) {
    // Optional: You can implement write support here using m_core
    qDebug() << "pvSetValue not implemented for OPC UA";
    return false;
}

// caQtDM_Lib will call this routine for setting waveforms data (see for more detail the epics3 plugin)
int OPCUAPlugin::pvSetWave(char *pv, float *fdata, double *ddata, int16_t *data16, int32_t *data32, char *sdata, int nelm, char *object, char *errmess) {
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
    qDebug() << "OPCUAPlugin:pvSetWave";
    return true;
}

// caQtDM_Lib will call this routine for getting a description of the monitor
int OPCUAPlugin::pvGetTimeStamp(char *pv, char *timestamp) {
    Q_UNUSED(pv);
    Q_UNUSED(timestamp);
    qDebug() << "OPCUAPlugin:pvgetTimeStamp";
    strcpy(timestamp, "timestamp in epics format");
    return true;
}

// caQtDM_Lib will call this routine for getting the timestamp for this monitor
int OPCUAPlugin::pvGetDescription(char *pv, char *description) {
    Q_UNUSED(pv);
    Q_UNUSED(description);
    qDebug() << "OPCUAPlugin:pvGetDescription";
    strcpy(description, "hello, I am a double");
    return true;
}

// next two routines are used to stop and restart the monitoring (used in case of tabWidgets in the display)
int OPCUAPlugin::pvClearEvent(void * ptr)
{

    knobData *kData = static_cast<knobData *>(ptr);
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData, endpoint, nodeId)) return false;

    QMutexLocker locker(&m_mutex);
    if (m_cores.contains(endpoint)) {
        m_cores[endpoint]->disableMonitoringForNode(nodeId);  // must exist in your OpcUaCore
        qDebug() << "OPCUAPlugin:pvClearEvent - paused monitoring for" << nodeId;
    }

    return true;
}

int OPCUAPlugin::pvAddEvent(void * ptr)
{

    knobData *kData = static_cast<knobData *>(ptr);
    QString endpoint, nodeId;
    if (!resolveConnectionString(kData, endpoint, nodeId)) return false;

    QMutexLocker locker(&m_mutex);
    if (m_cores.contains(endpoint)) {
        m_cores[endpoint]->subscribeToNode(nodeId);
        qDebug() << "OPCUAPlugin:pvAddEvent - resumed monitoring for" << nodeId;
    }

    return true;
}

// next two routines are used to connect and disconnect monitors when the application gest suspended and reactivated
int OPCUAPlugin::pvReconnect(knobData *kData)
{

    QString endpoint, nodeId;
    if (!resolveConnectionString(kData, endpoint, nodeId))
        return false;

    QMutexLocker lock(&m_mutex);
    if (!m_cores.contains(endpoint))
        return false;

    auto core = m_cores[endpoint];
    core->disconnectOpc();
    m_connectionState[endpoint] = ConnectionState::NotConnected;

    core->connectOpc(endpoint, [=](bool success) {
        if (success) {
            m_connectionState[endpoint] = ConnectionState::Connected;
            core->subscribeToNode(nodeId);
            qDebug() << "OPCUAPlugin: Reconnected and subscribed to" << nodeId;
        } else {
            qWarning() << "OPCUAPlugin: Failed to reconnect to" << endpoint;
        }
    });
    return true;
}

int OPCUAPlugin::pvDisconnect(knobData *kData)
{

    QString endpoint, nodeId;
    if (!resolveConnectionString(kData, endpoint, nodeId))
        return false;

    QMutexLocker lock(&m_mutex);
    if (m_cores.contains(endpoint)) {
        m_cores[endpoint]->disconnectOpc();
        m_connectionState[endpoint] = ConnectionState::NotConnected;
        qDebug() << "OPCUAPlugin: Disconnected from" << endpoint;
    }
    return true;
}

// flush any io is periodically called (1s timer) in order to flush the disconnection and reconnection
// used for pv's that will be hidden and shown in case of tabwidgets
int OPCUAPlugin::FlushIO() {
    //qDebug() << "OPCUAPlugin:FlushIO";
    return true;
}

// termination (in case of epics3, this is used to destroy the context when the application gest deactivated
// otherwise probably no meaning; in this demo, we stop the simulation, however it will not be reactivated
// any more (you may do that through pvReconnect)
int OPCUAPlugin::TerminateIO()
{
    qDebug() << "OPCUAPlugin: TerminateIO called.";
#if DBL_MANT_DIG < LDBL_MANT_DIG

 #endif
    QMutexLocker locker(&m_mutex);

    for (auto &core : m_cores) {
        if (core) {
            core->clearAllSubscriptions();  // gracefully disable monitoring
            core->disconnectOpc();          // disconnect from server
        }
    }

    m_cores.clear();                // remove references
    m_connectionState.clear();      // reset states
    m_pendingSubscriptions.clear(); // reset pending list
    Channelcache.clear();           // clear nodeId → index map

    return true;
}

QString OPCUAPlugin::findNodeIdByIndex(int index){
    for (auto it = Channelcache.begin(); it != Channelcache.end(); ++it) {
        if (it.value() == index)
            return it.key();
    }
    return QString();
}

bool OPCUAPlugin::resolveConnectionString(knobData *kData, QString &endpoint, QString &nodeId)
{
    QString logicalKey = QString::fromLatin1(kData->pv).remove("opcua://");
    QString fullConnection = opcua_translation_map.value(logicalKey, "");
    if(fullConnection.isEmpty()){
        qWarning() << "OPCUAPlugin: No translation found for " << logicalKey;
        return false;
    }

    QString raw = fullConnection.remove("opcua://");
    int splitPos = raw.lastIndexOf("/ns=");
    if(splitPos < 0){
        splitPos = raw.lastIndexOf("/i=");
        if(splitPos < 0){
            qWarning() << "OPCUAPlugin: Invalid connection string: " << fullConnection;
            return false;
        }
    }

    endpoint = raw.left(splitPos);
    nodeId = raw.mid(splitPos + 1).trimmed();
    return true;
}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#define QT_VARIANT_TYPE(value) value.typeId()
#else
#define QT_VARIANT_TYPE(value) value.type()
#endif

caType OPCUAPlugin::generateCaTypeFromVariant(const QVariant &value)
{
    switch (QT_VARIANT_TYPE(value)) {
    case QMetaType::Double:
        return caDOUBLE;
    case QMetaType::Float:
        return caFLOAT;
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Short:
    case QMetaType::Long:
    case QMetaType::ULong:
        return caINT;
    case QMetaType::Bool:
        return caENUM;
    case QMetaType::QString:
        return caSTRING;
    default:
        return caDOUBLE;
    }
}

void OPCUAPlugin::updateKnobDataFromVariant(knobData &kData, const QVariant &value)
{
    QMutexLocker locker((QMutex *)kData.mutex);

    caType detectedType = generateCaTypeFromVariant(value);
    kData.edata.fieldtype = detectedType;
    kData.edata.connected = 1;
    kData.edata.accessR = true;
    kData.edata.accessW = false;
    kData.edata.monitorCount++;

    switch (detectedType) {
    case caDOUBLE:
        kData.edata.rvalue = value.toDouble();
        kData.edata.precision=8;
        break;
    case caFLOAT:
        kData.edata.rvalue = value.toDouble();
        kData.edata.precision=4;
        break;
    case caINT:
        kData.edata.ivalue = value.toInt();
        kData.edata.precision=0;
        break;
    case caENUM:
        kData.edata.ivalue = value.toBool() ? 1.0 : 0.0;
        break;
    case caSTRING:
        if (kData.edata.dataB){
            if (kData.edata.dataSize != (value.toString().length()+1)){
                free(kData.edata.dataB);
                kData.edata.dataB=Q_NULLPTR;
            }
        }
        if (!kData.edata.dataB){
            kData.edata.dataSize = value.toString().length()+1;
            kData.edata.dataB = (void*)malloc((size_t)kData.edata.dataSize);
        }
        memcpy(kData.edata.dataB, (char*) value.toString().toLatin1().constData()
           , (size_t)kData.edata.dataSize);
        break;
    default:
        kData.edata.rvalue = 0.0;
        break;
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#else
Q_EXPORT_PLUGIN2(DemoPlugin, DemoPlugin)
#endif
