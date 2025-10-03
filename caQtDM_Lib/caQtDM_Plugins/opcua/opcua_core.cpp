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

#include "opcua_core.h"
#include "qeventloop.h"
#include "qrandom.h"
#include "qtcpsocket.h"
#include <QDebug>

namespace opc{
OpcUaCore::OpcUaCore(QObject *parent)
    : QObject(parent), m_client(nullptr)
{
    QOpcUaProvider* provider = new QOpcUaProvider();

    QStringList backends = provider->availableBackends();
    if(!backends.contains("open62541")){
        emit errorOccured("Open62541 not found. Please make sure the OPCUA QT Plugin is installed.");
    }

    m_client = provider->createClient("open62541");
    if (!m_client) {
        emit errorOccured("Failed to create OPC UA client instance.");
        return;
    }

    QObject::connect(m_client, &QOpcUaClient::connected, this, [this]() {
        emit connected();

        m_reconnecting = false; // stop ongoing reconnect attempts
        m_attempt = 0;
        m_timeoutMs = 100;

        for (auto it = m_subscriptionNodes.begin(); it != m_subscriptionNodes.end(); ++it) {
            QOpcUaNode *node = it.value();
            if (node) {
                startMonitoringOfNode(node);
            }
        }
    });


    QObject::connect(m_client, &QOpcUaClient::disconnected, this, [this]() {
        emit disconnected();

        if (m_reconnecting)
            return;

        m_reconnecting = true;
        m_attempt = 0;
        m_timeoutMs = 100;

        QTimer *reconnectTimer = new QTimer(this);
        reconnectTimer->setSingleShot(true);

        QObject::connect(reconnectTimer, &QTimer::timeout, this, [this, reconnectTimer]() {
            if (this->isClientConnected()) {
                m_reconnecting = false;
                reconnectTimer->deleteLater();
                return;
            }
            if (m_client->state() == QOpcUaClient::Connecting) {
                // Previous try is still going, restart the current timer.
                reconnectTimer->start(m_timeoutMs);
                return;
            }

            m_client->connectToEndpoint(m_currentEndpointDescription);
            m_attempt++;
            m_timeoutMs = qMin(m_timeoutMs * 2, 60000); // Slowly increase timeout

            reconnectTimer->start(m_timeoutMs);
        });

        reconnectTimer->start(0);
    });

    QObject::connect(m_client, &QOpcUaClient::errorChanged, this,
                     [this](QOpcUaClient::ClientError error) {
                         emit errorOccured(QString("Client error: %1").arg(static_cast<int>(error)));
                     });
}

OpcUaCore::~OpcUaCore()
{
    clearAllSubscriptions();
    QObject::disconnect(this);

    if (m_client) {
        QObject::disconnect(m_client);
        if (m_client->state() != QOpcUaClient::Disconnected) {
            QEventLoop loop;
            QObject::connect(m_client, &QOpcUaClient::disconnected, &loop, &QEventLoop::quit);
            m_client->disconnectFromEndpoint();
            loop.exec();
        }
        m_client->deleteLater();
    }
}


bool OpcUaCore::connectOpc(const QString &url)
{
    if (!m_client) {
        emit errorOccured("Client is not initialized.");
        return false;
    }

    QObject::connect(m_client, &QOpcUaClient::endpointsRequestFinished, this,
            [this, url](const QVector<QOpcUaEndpointDescription> &returnedEndpoints,
                                QOpcUa::UaStatusCode status,
                                const QUrl &) {
                // If no endpoints are returned at all, there is something fundamentally wrong with the server.
                // Thus, not even the fallbackEndpoint is checked from the pv, and we error out here.
                if (returnedEndpoints.isEmpty() || status != QOpcUa::UaStatusCode::Good) {
                    emit errorOccured("No endpoints received or status not good.");
                    return;
                }

                // Add a fallbackEndpoint which is from the provided pv string (url)
                QOpcUaEndpointDescription fallbackEndpoint = returnedEndpoints.constFirst();
                fallbackEndpoint.setEndpointUrl(url);
                int fallbackPort = QUrl(url).port(4840); // Fallback port is the port given in the pv string or 4840, if none given.

                QVector<QOpcUaEndpointDescription> endpoints = returnedEndpoints;
                endpoints.append(fallbackEndpoint);

                QOpcUaEndpointDescription chosenEndpoint;
                bool foundWorkingEndpoint = false;
                QList<QTcpSocket*> sockets;
                QEventLoop loop;
                QTimer timer;
                std::atomic<bool> found(false);

                timer.setSingleShot(true);
                QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

                // For each endpoint returned from the server, try to establish a simple tcp connection. The first endpoint that connects is chosen for further opcua communication.
                for (int i = 0; i < endpoints.size(); ++i) {
                    QOpcUaEndpointDescription ep = endpoints.at(i);
                    QUrl url = ep.endpointUrl();
                    QTcpSocket* sock = new QTcpSocket(this);
                    sockets.append(sock);

                    QObject::connect(sock, &QTcpSocket::connected, this,
                            [&]() {
                                if (found.exchange(true)) return;
                                chosenEndpoint = ep;
                                foundWorkingEndpoint = true;
                                timer.stop();
                                loop.quit();
                                for (QTcpSocket* s : sockets) if (s && s->state() == QAbstractSocket::ConnectedState && s != qobject_cast<QTcpSocket*>(QObject::sender())) s->abort();
                            });
                    ;
                    sock->connectToHost(url.host(), url.port(fallbackPort)); // Uses either endoint provided port, or defaults to fallbackPort
                }

                // Try to connect to all endpoints for a certain time. Due to signal / slot mechanism, the fastest connection will usually be chosen.
                timer.start(500);
                loop.exec();

                for (QTcpSocket* s : sockets) { if (s) { s->abort(); s->deleteLater(); } }

                if (!foundWorkingEndpoint) {
                    emit errorOccured("No reachable endpoint hosts.");
                    return;
                }

                m_client->connectToEndpoint(chosenEndpoint);
                m_currentEndpointDescription = chosenEndpoint;
        }, Qt::SingleShotConnection);

    m_client->requestEndpoints(url);
    return true;
}


void OpcUaCore::disconnectOpc()
{
    if (m_client &&(m_client->state() == QOpcUaClient::ClientState::Connected || m_client->state() == QOpcUaClient::ClientState::Connecting)) {
        qDebug() << "Disconnecting from OPC UA Server....";
        m_client->disconnectFromEndpoint();
    }else{
        qDebug() << "Client not connected or already disconnected.";
    }
}


void OpcUaCore::subscribeToNode(const SubscriptionSettings &subscriptionSettings)
{
    QString nodeId = subscriptionSettings.nodeid;
    int intervalMs = subscriptionSettings.samplingIntervalMs;

    if (!isClientConnected()) {
        emit errorOccured("Client is not connected.");
        return;
    }

    if (m_subscriptionNodes.contains(nodeId)) {
        qInfo() << "Already subscribed to node:" << nodeId;
        return;
    }

    QOpcUaNode *node = m_client->node(nodeId);
    if (!node) {
        emit errorOccured("Failed to create node object for subscription: " + nodeId);
        return;
    }
    m_subscriptionNodes.insert(nodeId, node);

    m_intervalMsForNodeId[nodeId] = intervalMs;

    startMonitoringOfNode(node);
}

void OpcUaCore::startMonitoringOfNode(QOpcUaNode *node) {
    QString nodeId = node->nodeId();
    if (m_isConnectingToNode[nodeId]) {
        return;
    }
    m_isConnectingToNode[nodeId] = true;
    int intervalMs = m_intervalMsForNodeId.value(nodeId, 10);

    QObject::connect(node, &QOpcUaNode::attributeRead, this, [=](QOpcUa::NodeAttributes attrs) {
        if (!attrs.testFlag(QOpcUa::NodeAttribute::NodeClass)) {
            emit errorOccured("Failed to read NodeClass for node: " + nodeId);
            node->deleteLater();
            return;
        }

        auto nodeClass = static_cast<QOpcUa::NodeClass>(
            node->attribute(QOpcUa::NodeAttribute::NodeClass).toInt());

        if (nodeClass != QOpcUa::NodeClass::Variable) {
            emit errorOccured("Node " + nodeId + " is not a Variable. Subscription aborted.");
            node->deleteLater();
            return;
        }

        // Enable monitoring
        QOpcUaMonitoringParameters params;
        params.setSamplingInterval(intervalMs);
        params.setMonitoringMode(QOpcUaMonitoringParameters::MonitoringMode::Reporting);
        params.setSubscriptionType(QOpcUaMonitoringParameters::SubscriptionType::Shared);

        if (!node->enableMonitoring(QOpcUa::NodeAttribute::Value, params)) {
            emit errorOccured("Failed to enable monitoring for node: " + nodeId);
            node->deleteLater();
            return;
        }

        QObject::connect(node, &QOpcUaNode::dataChangeOccurred, this,
                         [this, nodeId](QOpcUa::NodeAttribute attr, const QVariant &value) {
                             if (attr == QOpcUa::NodeAttribute::Value) {
                                 emit valueRead(nodeId, value);
                             }
                         });

        qDebug() << "[subscribeToNode] Subscribed successfully to:" << nodeId;

        QVariant accessLevel = node->attribute(QOpcUa::NodeAttribute::UserAccessLevel);

        if (accessLevel.isValid()) {
            bool readAccess = accessLevel.value<quint8>() & static_cast<quint8>(QOpcUa::AccessLevelBit::CurrentRead);
            bool writeAccess = accessLevel.value<quint8>() & static_cast<quint8>(QOpcUa::AccessLevelBit::CurrentWrite);
            emit accessLevelRead(nodeId, readAccess, writeAccess);
        }

        m_isConnectingToNode[nodeId] = false;
    });

    node->readAttributes(QOpcUa::NodeAttribute::NodeClass | QOpcUa::NodeAttribute::UserAccessLevel | QOpcUa::NodeAttribute::Value | QOpcUa::NodeAttribute::Description);
}


void OpcUaCore::clearAllSubscriptions()
{
    qInfo() << "Clearing all OPC UA subscriptions...";

    for (auto it = m_subscriptionNodes.begin(); it != m_subscriptionNodes.end(); ++it) {
        QOpcUaNode *node = it.value();
        if (node) {
            node->disableMonitoring(QOpcUa::NodeAttribute::Value);
            node->disconnect(); // disconnect any signals/slots
            unsubscribeFromNode(node->nodeId());
        }
    }

    m_subscriptionNodes.clear();
    qInfo() << "All OPC UA subscriptions have been cleared.";
}

// This method is usefull for when you don't know the nodeId's to check if
// the server actually has Data to fetch from.
void OpcUaCore::fetchDataFromAnyNode() {
    if (!isClientConnected())
        return;

    const QString objectsNodeId = QStringLiteral("ns=0;i=85"); // ns=0;i=85 is always the root in an opcua server.
    qDebug() << "fetchDataFromAnyNode: browsing Objects folder" << objectsNodeId;
    QOpcUaNode *objectsNode = m_client->node(objectsNodeId);
    if (!objectsNode) {
        emit errorOccured("fetchDataFromAnyNode: Failed to create browse node for " + objectsNodeId);
        return;
    }

    // 1) Browse for all OBJECT children of ns=0;i=85, ns=0;i=85 is always the root in an opcua server.
    QOpcUaBrowseRequest req;
    req.setBrowseDirection(QOpcUaBrowseRequest::BrowseDirection::Forward);
    req.setReferenceTypeId(QOpcUa::ReferenceTypeId::HierarchicalReferences);
    req.setIncludeSubtypes(true);
    // ONLY Objects here
    req.setNodeClassMask(QOpcUa::NodeClasses(QOpcUa::NodeClass::Object));

    QObject::connect(objectsNode, &QOpcUaNode::browseFinished, this,
            [this, objectsNode](QVector<QOpcUaReferenceDescription> children,
                                QOpcUa::UaStatusCode status) {
                objectsNode->deleteLater();

                if (status != QOpcUa::Good) {
                    emit errorOccured(QStringLiteral(
                                          "fetchDataFromAnyNode: browse of Objects folder failed: %1")
                                          .arg(QOpcUa::statusToString(status)));
                    return;
                }

                QStringList objectNodeIds;
                for (auto &ref : children) {
                    if (ref.nodeClass() == QOpcUa::NodeClass::Object)
                        objectNodeIds << ref.targetNodeId().nodeId();
                }

                if (objectNodeIds.isEmpty()) {
                    emit errorOccured(QStringLiteral(
                                          "fetchDataFromAnyNode: no Object nodes under %1")
                                          .arg(objectsNode->nodeId()));
                    return;
                }

                // pick one random Object and browse it for Variables
                const QString rndObject = objectNodeIds.at(QRandomGenerator::global()->bounded(objectNodeIds.size()));
                browseObjectForVariables(rndObject);
            }, Qt::AutoConnection);

    if (!objectsNode->browse(req)) {
        emit errorOccured("fetchDataFromAnyNode: failed to dispatch browse request");
        objectsNode->deleteLater();
    }
}

void OpcUaCore::browseObjectForVariables(const QString &objectNodeId) {
    QOpcUaNode *objNode = m_client->node(objectNodeId);
    if (!objNode) {
        emit errorOccured("browseObjectForVariables: failed to create node object for " + objectNodeId);
        return;
    }

    qDebug() << "browseObjectForVariables: browsing for Variables under" << objectNodeId;

    QOpcUaBrowseRequest req;
    req.setBrowseDirection(QOpcUaBrowseRequest::BrowseDirection::Forward);
    req.setReferenceTypeId(QOpcUa::ReferenceTypeId::HierarchicalReferences);
    req.setIncludeSubtypes(true);
    // now only Variables
    req.setNodeClassMask(QOpcUa::NodeClasses(QOpcUa::NodeClass::Variable));

    QObject::connect(objNode, &QOpcUaNode::browseFinished, this,
            [this, objNode](QVector<QOpcUaReferenceDescription> children,
                            QOpcUa::UaStatusCode status) {
                objNode->deleteLater();

                if (status != QOpcUa::Good) {
                    emit errorOccured(QStringLiteral(
                                          "browseObjectForVariables: browse of %1 failed: %2")
                                          .arg(objNode->nodeId(),
                                               QOpcUa::statusToString(status)));
                    return;
                }

                if (children.isEmpty()) {
                    emit errorOccured(QStringLiteral(
                                          "browseObjectForVariables: no Variable nodes under %1")
                                          .arg(objNode->nodeId()));
                    return;
                }

                // pick one random Variable and read it
                const int idx = QRandomGenerator::global()->bounded(children.size());
                const QString variableNodeId = children.at(idx).targetNodeId().nodeId();
                qDebug() << "browseObjectForVariables: selected Variable node" << variableNodeId;
                fetchDataFromSingleNode(variableNodeId);
            }, Qt::AutoConnection);

    if (!objNode->browse(req)) {
        emit errorOccured("browseObjectForVariables: failed to dispatch browse request for " + objectNodeId);
        objNode->deleteLater();
    }
}

void OpcUaCore::fetchDataFromSingleNode(const QString &nodeId)
{
    // Check if we actually are connected.
    if(!m_client || m_client->state() != QOpcUaClient::Connected){
        emit errorOccured("Client is not connected.");
        return;
    }

    qInfo() << "Attempting to get handle for NodeId: " << nodeId;

    // Create a handle
    QOpcUaNode *node = m_client->node(nodeId);
    if(!node){
        emit errorOccured("Failed to create node object." + nodeId);
        return;
    }

    qInfo() << "Node object created for " << nodeId << ". Setting up and read.";

    // Handling
    QObject::connect(node, &QOpcUaNode::attributeRead, this, [this, node, nodeId](QOpcUa::NodeAttributes attrs){
        qInfo() << "attributeRead signal received for node:" << nodeId << "with attributes:" << attrs;

        // Check if the Value attribute was part of this read operation's response.
        // readValueAttribute() specifically requests the Value attribute.
        if (attrs.testFlag(QOpcUa::NodeAttribute::Value)) {
            qInfo() << "Value attribute is present in the response for node" << nodeId;
            // Now check the specific status code for the Value attribute.
            QOpcUa::UaStatusCode valueStatus = node->attributeError(QOpcUa::NodeAttribute::Value);
            if (valueStatus != QOpcUa::UaStatusCode::Good) {
                emit errorOccured(QString("Reading Value attribute for node %1 failed with status %2")
                                      .arg(nodeId)
                                      .arg(static_cast<int>(valueStatus)));
            } else {
                qInfo() << "Value attribute read successfully for node" << nodeId << ". Fetching value...";
                QVariant val = node->attribute(QOpcUa::NodeAttribute::Value);
                qDebug() << "Read value for node " << nodeId << ":" << val;
                emit valueRead(nodeId, val);
            }
        } else {
            // This case might occur if the server, despite the request, couldn't provide the Value attribute
            // or if the read operation failed at a lower level before even attempting to get attributes.
            emit errorOccured("Read response from server did not include the Value attribute for node: " + nodeId);
        }
        node->deleteLater();
    });


    int req = node->readValueAttribute();
    if(req < 0){
        emit errorOccured("Failed to dispatch readValueAttribtue()");
        node->deleteLater();
    }

}

void OpcUaCore::fetchDataFromMultipleNodes(const QStringList &nodeIds)
{
    for(auto &node : nodeIds){
        fetchDataFromSingleNode(node);
    }
}

bool OpcUaCore::isClientConnected(){
    if(!m_client || m_client->state() != QOpcUaClient::Connected){
        emit errorOccured("Client is not connected.");
        return false;
    }
    return true;
}

bool OpcUaCore::hasSubscription(const QString &nodeId) const {
    return m_subscriptionNodes.contains(nodeId);
}

void OpcUaCore::unsubscribeFromNode(const QString &nodeId) {
    if (!m_subscriptionNodes.contains(nodeId))
        return;

    m_intervalMsForNodeId.remove(nodeId);
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    m_subscriptionNodes.remove(nodeId);

    if (node ) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
        if (!(m_client && m_client->deleteNode(node->nodeId()))) {
            node->deleteLater();
        }
    }
}

void OpcUaCore::disableMonitoringForNode(const QString &nodeId){
    if (!m_subscriptionNodes.contains(nodeId)) return;
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (node) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#define QT_VARIANT_TYPE(value) value.typeId()
#else
#define QT_VARIANT_TYPE(value) value.type()
#endif

bool OpcUaCore::writeValue(const QString &nodeId, double rdata, int32_t idata, char *sdata,
                           char *errmess, int forceType) {
    Q_UNUSED(forceType);
    Q_UNUSED(errmess)

    if (!m_subscriptionNodes.contains(nodeId)) {
        emit errorOccured("Node not found");
        return false;
    }

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        emit errorOccured("Node is null");
        return false;
    }

    auto makeValue = [&](const QVariant &ref) -> QVariant {
        switch (QT_VARIANT_TYPE(ref)) {
        case QMetaType::Double:
        case QMetaType::Float: return QVariant::fromValue<double>(rdata);
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Long:
        case QMetaType::ULong: return QVariant::fromValue<int32_t>(idata);
        case QMetaType::Short: return QVariant::fromValue<int16_t>(idata);
        case QMetaType::Bool: return QVariant::fromValue<bool>(idata != 0);
        case QMetaType::QString: return QString::fromUtf8(sdata ? sdata : "");
        default: return {};
        }
    };

    auto doWrite = [&](const QVariant &ref) {
        QVariant valueToWrite = makeValue(ref);
        if (!valueToWrite.isValid()) {
            emit errorOccured("Unsupported type");
            return;
        }

        QObject::connect(node, &QOpcUaNode::attributeWritten, this,
            [=](QOpcUa::NodeAttribute attr, QOpcUa::UaStatusCode status) {
                if (attr != QOpcUa::NodeAttribute::Value) return;
                if (status != QOpcUa::UaStatusCode::Good && errmess) {
                    emit errorOccured(QString("Write failed: %1")
                                        .arg(QOpcUa::statusToString(status))
                                        .toUtf8().constData());
                }
            },
            Qt::SingleShotConnection);

        node->writeValueAttribute(valueToWrite);
    };

    QVariant existingValue = node->attribute(QOpcUa::NodeAttribute::Value);
    if (existingValue.isValid()) {
        doWrite(existingValue);
        return true;
    }

    QObject::connect(node, &QOpcUaNode::attributeRead, this,
        [=](QOpcUa::NodeAttributes attrs) {
            if (!attrs.testFlag(QOpcUa::NodeAttribute::Value)) {
                emit errorOccured("Value not readable");
                return;
            }
            doWrite(node->attribute(QOpcUa::NodeAttribute::Value));
        },
        Qt::SingleShotConnection);

    node->readValueAttribute();
    return true;
}

bool OpcUaCore::writeValues(const QString &nodeId,
                            float *fdata,
                            double *ddata,
                            int16_t *data16,
                            int32_t *data32,
                            char *sdata,
                            int nelm,
                            char *errmess)
{
    Q_UNUSED(errmess)

    if (!m_subscriptionNodes.contains(nodeId)) {
        emit errorOccured("Node not found");
        return false;
    }

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        emit errorOccured("Node is null");
        return false;
    }

    auto makeValue = [&](const QVariant &ref) -> QList<QVariant> {
        QList<QVariant> values;
        values.reserve(nelm);


        switch (QT_VARIANT_TYPE(ref)) {
        case QMetaType::Double:
            for (int i = 0; i < nelm; ++i) values.append(ddata[i]);
            break;
        case QMetaType::Float:
            for (int i = 0; i < nelm; ++i) values.append(fdata[i]);
            break;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Long:
        case QMetaType::ULong:
            for (int i = 0; i < nelm; ++i) values.append(QVariant::fromValue<int32_t>(data32[i]));
            break;
        case QMetaType::Short:
            for (int i = 0; i < nelm; ++i) values.append(QVariant::fromValue<int16_t>(data16[i]));
            break;
        case QMetaType::Bool:
            for (int i = 0; i < nelm; ++i) values.append(QVariant::fromValue<bool>(data16[i] != 0));
            break;
        case QMetaType::QString:
            values.append(QString::fromUtf8(sdata ? sdata : ""));
            break;
        default:
            break;
        }

        return values;
    };

    auto doWrite = [&](const QVariant &ref) {
        QVariant valueToWrite = makeValue(ref);
        if (!valueToWrite.isValid()) {
            emit errorOccured("Unsupported type");
            return;
        }

        QObject::connect(node, &QOpcUaNode::attributeWritten, this,
            [=](QOpcUa::NodeAttribute attr, QOpcUa::UaStatusCode status) {
                if (attr != QOpcUa::NodeAttribute::Value) return;
                if (status != QOpcUa::UaStatusCode::Good && errmess) {
                    emit errorOccured(QString("Write failed: %1")
                                          .arg(QOpcUa::statusToString(status))
                                          .toUtf8().constData());
                }
            },
            Qt::SingleShotConnection);

        node->writeValueAttribute(valueToWrite);
    };

    QVariant existingValue = node->attribute(QOpcUa::NodeAttribute::Value);
    if (existingValue.isValid()) {
        doWrite(existingValue.toList().constFirst());
        return true;
    }

    QObject::connect(node, &QOpcUaNode::attributeRead, this,
        [=](QOpcUa::NodeAttributes attrs) {
            if (!attrs.testFlag(QOpcUa::NodeAttribute::Value)) {
                emit errorOccured("Value not readable");
                return;
            }
            doWrite(node->attribute(QOpcUa::NodeAttribute::Value).toList().constFirst());
        },
        Qt::SingleShotConnection);

    node->readValueAttribute();
    return true;
}

QString OpcUaCore::getDescription(const QString &nodeId) {
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        emit errorOccured("Node is null");
        return "Node is null";
    }
    return node->attribute(QOpcUa::NodeAttribute::Description).value<QOpcUaLocalizedText>().text();
}
}
