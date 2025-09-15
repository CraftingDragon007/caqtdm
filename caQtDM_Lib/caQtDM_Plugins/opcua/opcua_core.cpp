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
    m_client = m_provider.createClient("open62541");

    QStringList backends = m_provider.availableBackends();

    if (!m_client) {
        emit errorOccured("Failed to create OPC UA client instance.");
        return;
    }

    if(!backends.contains("open62541")){
        emit errorOccured("Open62541 not found. Please make sure the OPCUA QT Plugin is installed.");
    }
    connect(m_client, &QOpcUaClient::connected, this, &OpcUaCore::connected);
    connect(m_client, &QOpcUaClient::disconnected, this, &OpcUaCore::disconnected);
    connect(m_client, &QOpcUaClient::errorChanged, this,
            [this](QOpcUaClient::ClientError error) {
                emit errorOccured(QString("Client error: %1").arg(static_cast<int>(error)));
            });

}

OpcUaCore::~OpcUaCore()
{
    clearAllSubscriptions();

    if (m_client) {
        if (m_client->state() != QOpcUaClient::Disconnected) {
            QEventLoop loop;
            connect(m_client, &QOpcUaClient::disconnected, &loop, &QEventLoop::quit);
            m_client->disconnectFromEndpoint();
            loop.exec();
        }
        delete m_client;
        m_client = nullptr;
    }
}


bool OpcUaCore::connectOpc(const QString &url, std::function<void(bool)> onConnected)
{
    if (!m_client) {
        emit errorOccured("Client is not initialized.");
        onConnected(false);
        return false;
    }

    if (!m_endpointsHooked) {
        connect(m_client, &QOpcUaClient::endpointsRequestFinished, this,
                [this, onConnected](const QVector<QOpcUaEndpointDescription> &endpoints,
                                    QOpcUa::UaStatusCode status,
                                    const QUrl &) {
                    if (endpoints.isEmpty() || status != QOpcUa::UaStatusCode::Good) {
                        emit errorOccured("No endpoints received or status not good.");
                        onConnected(false);
                        return;
                    }

                    QOpcUaEndpointDescription chosenEndpoint;
                    bool foundWorkingEndpoint = false;
                    QList<QTcpSocket*> sockets;
                    QEventLoop loop;
                    QTimer timer;
                    std::atomic<bool> found(false);

                    timer.setSingleShot(true);
                    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

                    // For each endpoint returned from the server, try to establish a simple tcp connection. The first endpoint that connects is chosen for further opcua communication.
                    for (int i = 0; i < endpoints.size(); ++i) {
                        QOpcUaEndpointDescription ep = endpoints.at(i);
                        QUrl url = ep.endpointUrl();
                        QTcpSocket* sock = new QTcpSocket(this);
                        sockets.append(sock);

                        connect(sock, &QTcpSocket::connected, this,
                                [this, ep, &sockets, &found, &chosenEndpoint, &foundWorkingEndpoint, &timer, &loop]() {
                                    if (found.exchange(true)) return;
                                    chosenEndpoint = ep;
                                    foundWorkingEndpoint = true;
                                    timer.stop();
                                    loop.quit();
                                    for (QTcpSocket* s : sockets) if (s && s->state() == QAbstractSocket::ConnectedState && s != qobject_cast<QTcpSocket*>(QObject::sender())) s->abort();
                                });

                        sock->connectToHost(url.host(), url.port(4840));
                    }

                    // Try to connect to all endpoints for a certain time. Due to signal / slot mechanism, the fastest connection will usually be chosen.
                    timer.start(500);
                    loop.exec();

                    for (QTcpSocket* s : sockets) { if (s) { s->abort(); s->deleteLater(); } }

                    if (!foundWorkingEndpoint) {
                        emit errorOccured("No reachable endpoint hosts.");
                        onConnected(false);
                        return;
                    }

                    connect(m_client, &QOpcUaClient::stateChanged, this,
                            [this, onConnected](QOpcUaClient::ClientState state) {
                                if (state == QOpcUaClient::Connected) {
                                    onConnected(true);
                                } else if (state == QOpcUaClient::Disconnected) {
                                    emit errorOccured("Disconnected during connection.");
                                    onConnected(false);
                                }
                            });

                    m_client->connectToEndpoint(chosenEndpoint);
                });
        m_endpointsHooked = true;
    }

    m_client->requestEndpoints(url);
    return true;
}


bool OpcUaCore::connectOpc(const QString &url)
{
    if (!m_client) {
        emit errorOccured("Client is not initialized.");
        return false;
    }

    if (!m_endpointsHooked) {
        connect(m_client, &QOpcUaClient::endpointsRequestFinished, this,
                [this](const QVector<QOpcUaEndpointDescription> &endpoints,
                       QOpcUa::UaStatusCode status,
                       const QUrl &) {
                    if (endpoints.isEmpty() || status != QOpcUa::UaStatusCode::Good) {
                        emit errorOccured("No endpoints received or status not good.");
                        return;
                    }
                    m_client->connectToEndpoint(endpoints.first());
                });
        m_endpointsHooked = true;
    }

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


void OpcUaCore::subscribeToNode(const QString &nodeId)
{
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

    // Connect first, then request attributes
    connect(node, &QOpcUaNode::attributeRead, this, [=](QOpcUa::NodeAttributes attrs) {
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
        params.setSamplingInterval(1000.0);
        params.setMonitoringMode(QOpcUaMonitoringParameters::MonitoringMode::Reporting);
        params.setSubscriptionType(QOpcUaMonitoringParameters::SubscriptionType::Shared);

        if (!node->enableMonitoring(QOpcUa::NodeAttribute::Value, params)) {
            emit errorOccured("Failed to enable monitoring for node: " + nodeId);
            node->deleteLater();
            return;
        }

        connect(node, &QOpcUaNode::dataChangeOccurred, this,
                [this, nodeId](QOpcUa::NodeAttribute attr, const QVariant &value) {
                    if (attr == QOpcUa::NodeAttribute::Value) {
                        emit valueRead(nodeId, value);
                    }
                });

        m_subscriptionNodes.insert(nodeId, node);
        qDebug() << "[subscribeToNode] Subscribed successfully to:" << nodeId;
    });

    // Must come after connect
    node->readAttributes(QOpcUa::NodeAttribute::NodeClass);
}


void OpcUaCore::clearAllSubscriptions()
{
    qInfo() << "Clearing all OPC UA subscriptions...";

    for (auto it = m_subscriptionNodes.begin(); it != m_subscriptionNodes.end(); ++it) {
        QOpcUaNode *node = it.value();
        if (node) {
            node->disableMonitoring(QOpcUa::NodeAttribute::Value);
            node->disconnect(); // disconnect any signals/slots
            node->deleteLater();
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

    connect(objectsNode, &QOpcUaNode::browseFinished, this,
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

    connect(objNode, &QOpcUaNode::browseFinished, this,
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
    connect(node, &QOpcUaNode::attributeRead, this, [this, node, nodeId](QOpcUa::NodeAttributes attrs){
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
    }, Qt::UniqueConnection);


    int req = node->readValueAttribute();
    if(req < 0){
        emit errorOccured("Failed to dispatch readValueAttribtue()");
        node->deleteLater();
    }

}

void OpcUaCore::fetchDataFromMultipleNodes(const QStringList &nodeIds)
{
    for(auto node : nodeIds){
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

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (node) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
        node->deleteLater();
    }
    m_subscriptionNodes.remove(nodeId);
}

void OpcUaCore::disableMonitoringForNode(const QString &nodeId){
    if (!m_subscriptionNodes.contains(nodeId)) return;
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (node) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
    }
}

}
