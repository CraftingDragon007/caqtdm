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

#ifndef OPCUA_CORE_H
#define OPCUA_CORE_H

#include <QObject>
#include <QUrl>
#include <QtOpcUa/QOpcUaAddReferenceItem>
#include <QtOpcUa/QOpcUaBrowseRequest>
#include <QtOpcUa/QOpcUaClient>
#include <QtOpcUa/QOpcUaEndpointDescription>
#include <QtOpcUa/QOpcUaExpandedNodeId>
#include <QtOpcUa/QOpcUaLocalizedText>
#include <QtOpcUa/QOpcUaNode>
#include <QtOpcUa/QOpcUaProvider>
#include <QtOpcUa/QOpcUaQualifiedName>
#include <QtOpcUa/QOpcUaReferenceDescription>

#define VERBOSELOG(msg) qDebug().nospace() << "[" << __FUNCTION__ << "]: " << msg

typedef struct
{
    QString nodeid;
    int samplingIntervalMs;
} SubscriptionSettings;

typedef struct
{
    QString username;
    QString password;
} PasswordCredentials;

class OpcUaCore : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Creates the core, instantiates a QOpcUaClient, connects signal/slots for error and reconnection handling
     * @param parent: parent object passed to super (QObject) constructor
     */
    explicit OpcUaCore(QObject *parent = Q_NULLPTR);
    ~OpcUaCore();

    /**
     * @brief Tries to connect to the given endpoint
     * Following the OpcUa protocol, first all available endpoints are fetched by the provided url. Then, a connection is established to the endpoint which connects the quickest.
     * @param url: url of the endpoint to connect to
     * @return true if connection attemt was started, else false (may still fail)
     */
    bool connectOpc(const QString &url);
    /**
     * @brief Disconnects from a given endpoint
     */
    void disconnectOpc();
    /**
     * @brief Subscribes to a node and starts monitoring it
     * @param subscriptionSettings: A SubscriptionSettings struct containing nodeId and fetch interval
     */
    void subscribeToNode(const SubscriptionSettings &subscriptionSettings);
    /**
     * @brief Disconnects all nodes (variables) and unsubscribes from them, "forgetting" about them completely
     */
    void clearAllSubscriptions();
    /**
     * @brief Checks whether or not a nodeId is stored for subscribing
     * @param nodeId: nodeId to check
     * @return true if nodeId is stored as a subscription node, else false
     */
    bool hasSubscription(const QString &nodeId) const;
    /**
     * @brief Unsubscribes from a node and disables monitoring
     * @param nodeId: the nodeId to unsubscribe from
     */
    void unsubscribeFromNode(const QString &nodeId);
    /**
     * @brief disables monitoring for a node
     * @param nodeId: the nodeId to disable monitoring for
     */
    void disableMonitoringForNode(const QString &nodeId);
    /**
     * @brief Tries to write a simple value via OpcUa, based on previously received value from the node
     * @param nodeId: nodeId to write the value to
     * @param rdata: double data (8 bytes)
     * @param idata: integer data (is cast to int16_t or int32_t based on previous value)
     * @param sdata: string data
     * @param errmess: output an optional error message is copied to
     * @return true if the write was initiated successfully, false if not (may still fail)
     */
    bool writeValue(const QString &nodeId, double rdata, int32_t idata, char *sdata, char *errmess);
    /**
     * @brief Tries to write a 1D-Array of simple values via OpcUa, based on previously received value from the node
     * @param nodeId: nodeId to write the value to
     * @param fdata: float data
     * @param fdata: float (4 byte per var) data
     * @param ddata: double (8 byte per var) data
     * @param data16: int16_t data
     * @param data32: int32_t data
     * @param sdata: string data
     * @param nelm: number of elements
     * @param errmess: output an optional error message is copied to
     * @return true if the write was initiated successfully, false if not (may still fail)
     */
    bool writeValues(const QString &nodeId,
                     float *fdata,
                     double *ddata,
                     int16_t *data16,
                     int32_t *data32,
                     char *sdata,
                     int nelm,
                     char *errmess);
    /**
     * @brief Returns the OpcUa description field
     * @param nodeId: nodeId to get the description for
     * @return Description
     */
    QString getDescription(const QString &nodeId);
    /**
     * @brief Returns the timestamp for the last value received via OpcUa
     * @param nodeId: nodeId to get the timestamp for
     * @return Timestamp with date and time, millisecond-precision
     */
    QString getTimestamp(const QString &nodeId);

    /**
     * @brief Updates the given credentials and restarts the connection for all nodes handled by this core
     * @param newPasswordCredentials: the new password credentials to use. Will always replace old credentials entirely.
     */
    void updatePasswordCredentials(const PasswordCredentials &newPasswordCredentials);

    /**
     * @brief Sets the password used for decrypting the PEM
     */
    void setPemPassword(const QString &newPassword);
signals:
    /**
     * @brief Emitted when the OpcUa client has successfully connected to an endpoint
     */
    void connected();
    /**
     * @brief Emitted when the OpcUa client has been disconnected from and endpoint, no matter the reason
     */
    void disconnected();
    /**
     * @brief Emitted when a simple value monitored from a subscription was read via OpcUa
     * @param nodeId: nodeId the value was read for
     * @param value: Value itself
     */
    void valueRead(const QString &nodeId, const QVariant &value);
    /**
     * @brief Emitted when an array value monitored from a subscription was read via OpcUa
     * @param values: Array of values read
     */
    void valuesRead(const QVector<QVariant> &values);
    /**
     * @brief Emitted when the access level was read for a nodeId
     * @param nodeId: the nodeId the access level was read for
     * @param readAccess: Whether or not read access is possible
     * @param writeAccess Whether or not write access is possible
     */
    void accessLevelRead(const QString &nodeId, const bool &readAccess, const bool &writeAccess);
    /**
     * @brief Emitted when a value monitored from a subscription was read as invalid via OpcUa
     * @param nodeId: the nodeId the value was read for
     * @param errorMsg: Explanation / status code
     */
    void attributeGotError(const QString &nodeId, const QString &errorMsg);

private:
    QOpcUaClient *m_client;
    QString m_pemPassword;
    // Most recent url specified for connection
    QUrl m_latestEndpointUrl;
    // Holds information about the endpoint the core is currently connected to
    QOpcUaEndpointDescription m_currentEndpointDescription;
    // Map of nodeId to node with all nodes that are configured to be subscribed to
    QMap<QString, QOpcUaNode *> m_subscriptionNodes;
    // Map of nodeId to wether or not the nodeId is currently being connected to
    QSet<QString> m_activelyMonitoredNodes;
    // Map of nodeId to interval in Ms for its subscription
    QMap<QString, int> m_intervalMsForNodeId;
    PasswordCredentials m_passwordCredentials;
    int m_reconnectionAttempt;
    int m_reconnectionTimeoutMs;
    bool m_reconnecting;
    bool m_ignoreNextDisconnect;

    /**
     * @brief Gets the Qt standard path for AppLocalDataLocation and uses that to form a path for PKI configurations
     * No path is created here, it simply returns the string.
     * @return The path to use for PKI configurations
     */
    static QString defaultPkiPath();

    /**
     * @brief Deletes all pki configuration stored to the default PKI path
     */
    void clearPkiConfig();

    /**
     * @brief Sets up a valid PKI configuration under the standard path
     */
    void setupPkiConfig();

    /**
     * @brief Tries to connecto to all given endpoints via tcp and approximately returns the one with the fastes response time
     * @param endpointDescriptions: the list of endpoint(-description)s to check
     * @return The fastests endpoint (as a description) or a description with an empty endpointUrl in case none replied within a certain timeout
     */
    QOpcUaEndpointDescription getEndpointWithLowestLatency(
        const QVector<QOpcUaEndpointDescription> &endpointDescriptions);

    /**
     * @brief Chooses an appropriate endpoint out of many, considering connectivity and security & what the client supports in it's current configuration
     * @param endpointDescriptions: the list of endpoint(-description)s to choose from (output from endpointsRequest)
     * @param fallbackUrl: the fallback url to include in the checks as an alternative to the list
     * This is useful e.g. for port-forwarding or domain-aliasing where the host is reachable over a different URI than it knows itself
     * @return The chosen endpoint description, or if none was suitable an endpoint description with an empty endpointUrl
     */
    QOpcUaEndpointDescription chooseEndpointDescription(
        const QVector<QOpcUaEndpointDescription> &endpointDescriptions, const QUrl &fallbackUrl);
    /**
     * @brief Checks if the clients state is connected
     * @return true if the client is initialized and its state() is Connected, else false
     */
    bool isClientConnected();
    /**
     * @brief Starts monitoring an already configured node
     * @param node: Node to monitor
     */
    void startMonitoringOfNode(QOpcUaNode *node);
    /**
     * @brief Checks the previously read value from a node and updates it using the provided function
     * @param node: Node to read/update the value for
     * @param makeValue: Function that takes the existing value and returns the new value to be written
     * @return true if the update was successfully initiated, else false (update may still be rejected from OpcUa)
     */
    bool writeDataDynamically(QOpcUaNode *node, std::function<QVariant(const QVariant &)> makeValue);
};

#endif // OPCUA_CORE_H
