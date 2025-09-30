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

#ifndef OPCUA_CLIENT_H
#define OPCUA_CLIENT_H

#include <QObject>
#include <QtOpcUa/QOpcUaClient>
#include <QtOpcUa/QOpcUaProvider>
#include <QtOpcUa/QOpcUaNode>
#include <QtOpcUa/QOpcUaAddReferenceItem>
#include <QtOpcUa/QOpcUaExpandedNodeId>
#include <QtOpcUa/QOpcUaClient>
#include <QtOpcUa/QOpcUaEndpointDescription>
#include <QtOpcUa/QOpcUaBrowseRequest>
#include <QtOpcUa/QOpcUaReferenceDescription>
#include <QtOpcUa/QOpcUaQualifiedName>
#include <QtOpcUa/QOpcUaLocalizedText>
#include <QUrl>
#include <QTimer>

namespace opc{

typedef struct {
    QString nodeid;
    int samplingIntervalMs;
} SubscriptionSettings;

class OpcUaCore : public QObject
{
    Q_OBJECT

public:
    explicit OpcUaCore(QObject *parent = nullptr);
    ~OpcUaCore();

    bool connectOpc(const QString &url);
    void disconnectOpc();
    void fetchDataFromAnyNode();
    void fetchDataFromSingleNode(const QString &nodeId);
    void fetchDataFromMultipleNodes(const QStringList &nodeIds);
    void subscribeToNode(const SubscriptionSettings &subscriptionSettings);
    void subscribeToMultipleNodes(const QStringList &nodeIds);
    void clearAllSubscriptions();
    void browseRoot();
    bool hasSubscription(const QString &nodeId) const;
    void unsubscribeFromNode(const QString &nodeId);
    void disableMonitoringForNode(const QString &nodeId);
    bool writeValue(const QString &nodeId, double rdata, int32_t idata, char *sdata, char *object, char *errmess, int forceType);
    QString getDescription(const QString &nodeId);

signals:
    void connected();
    void disconnected();
    void errorOccured(const QString message);
    void valueRead(const QString nodeId, const QVariant value);
    void valuesRead(const QVector<QVariant> &values);
    void accessLevelRead(const QString nodeId, const bool readAccess, const bool writeAccess);

private:
    QOpcUaClient *m_client;
    QOpcUaEndpointDescription m_currentEndpointDescription;
    int m_attempt;
    int m_timeoutMs;
    bool m_reconnecting;
    bool m_endpointsHooked = false;
    bool isClientConnected();
    void browseObjectForVariables(const QString &objectNodeId);
    QMap<QString, QOpcUaNode*> m_subscriptionNodes;
    QMap<QString, bool> m_isConnectingToNode;
    QMap<QString, int> m_intervalMsForNodeId;
    void QOpcUaBrowseResult(QString nodeId, QOpcUaNode *, void (*)(QVector<QOpcUaReferenceDescription>, QOpcUa::UaStatusCode), OpcUaCore *, QDebug);
    void startMonitoringOfNode(QOpcUaNode *node);
};
}
#endif // OPCUA_CLIENT_H
