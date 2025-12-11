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
#include <QDebug>
#include <QStandardPaths>
#include <QTimer>
#include "qdir.h"
#include "qeventloop.h"
#include "qmetaobject.h"
#include "qopcuaauthenticationinformation.h"
#include "qtcpsocket.h"
#include "x509certificate.h"
#include <qopcuaerrorstate.h>

#define DEFAULT_OPCUA_PORT 4840
#define INITIAL_RECONNECTION_TIMEOUT 100
#define RECONNECTION_TIMEOUT_FACTOR 2
#define MAX_RECONNECTION_TIMEOUT 60000

#define DEFAULT_MAX_LATENCY 500

#define NOPASS_PLACEHOLDER "caQtDM"

OpcUaCore::OpcUaCore(QObject *parent)
    : QObject(parent)
    , m_client(Q_NULLPTR)
    , m_pemPassword("")
    , m_passwordCredentials({"", ""})
{
    QOpcUaProvider provider;

    QStringList backends = provider.availableBackends();
    if (!backends.contains("open62541")) {
        VERBOSELOG("Open62541 not found.");
        return;
    }

    m_client = provider.createClient("open62541");
    if (!m_client) {
        VERBOSELOG("Failed to create OPC UA client instance.");
        return;
    }

    QString username = qgetenv("CAQTDM_OPCUA_USERNAME_PLAIN");
    QString password = qgetenv("CAQTDM_OPCUA_PASSWORD_PLAIN");
    if (!username.isEmpty() && !password.isEmpty()) {
        QOpcUaAuthenticationInformation authInfo;
        authInfo.setUsernameAuthentication(username, password);
        m_client->setAuthenticationInformation(authInfo);
        m_passwordCredentials = {username, password};
    }

    if (!qgetenv("CAQTDM_OPCUA_RESET_PKI_CONFIG").isEmpty()) {
        VERBOSELOG("Resetting PKI Config.");
        clearPkiConfig();
    }

    setupPkiConfig();

    QObject::connect(m_client,
                     &QOpcUaClient::passwordForPrivateKeyRequired,
                     this,
                     [this](QString keyFilePath, QString *password, bool previousTryWasInvalid) {
                         Q_UNUSED(keyFilePath);
                         if (previousTryWasInvalid) {
                             if (*password != NOPASS_PLACEHOLDER) {
                                 // Maybe the user specified a password but this pki config was created without one
                                 VERBOSELOG(
                                     "Failed to decrypt private key with given password, trying "
                                     "default. To reset, specify CAQTDM_OPCUA_RESET_PKI_CONFIG.");
                                 *password = NOPASS_PLACEHOLDER;
                                 return;
                             }
                             VERBOSELOG("Failed to decrypt private key, have you specified a "
                                        "password when initializing it via environment variable? "
                                        "To reset, specify CAQTDM_OPCUA_RESET_PKI_CONFIG.");
                             *password = "";
                             return;
                         }

                         if (!m_pemPassword.isEmpty()) {
                             *password = m_pemPassword;
                             VERBOSELOG("Using explicitely provided password via "
                                        "opcua://pem_password for decrypting pem.");
                             return;
                         }

                         QString pemPassword = qgetenv("CAQTDM_OPCUA_PEM_PASSWORD");
                         if (pemPassword.isEmpty()) {
                             pemPassword = NOPASS_PLACEHOLDER;
                         }
                         *password = pemPassword;
                     });

    QObject::connect(m_client, &QOpcUaClient::connected, this, [this]() {
        emit connected();
        m_reconnecting = false; // stop ongoing reconnect attempts
        m_reconnectionAttempt = 0;
        m_reconnectionTimeoutMs = INITIAL_RECONNECTION_TIMEOUT;

        for (QOpcUaNode *node : m_subscriptionNodes) {
            if (node) {
                // Start monitoring, will not do anything if it is already connected. Used in case of previous reconnects.
                startMonitoringOfNode(node);
            }
        }
    });

    // Handler to reconnect upon disconnections
    QObject::connect(m_client, &QOpcUaClient::disconnected, this, [this]() {
        emit disconnected();

        if (m_reconnecting)
            return;

        m_reconnecting = true;
        m_reconnectionAttempt = 0;
        m_reconnectionTimeoutMs = INITIAL_RECONNECTION_TIMEOUT;

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
                reconnectTimer->start(m_reconnectionTimeoutMs);
                return;
            }

            m_client->connectToEndpoint(m_currentEndpointDescription);
            m_reconnectionAttempt++;
            m_reconnectionTimeoutMs = qMin(
                m_reconnectionTimeoutMs * RECONNECTION_TIMEOUT_FACTOR,
                MAX_RECONNECTION_TIMEOUT); // Timeout is multiplied on each retry until some maxium timeout is reached

            reconnectTimer->start(m_reconnectionTimeoutMs);
        });

        reconnectTimer->start(0);
    });

    QObject::connect(
        m_client, &QOpcUaClient::errorChanged, this, [this](QOpcUaClient::ClientError error) {
            QString errorMessage = "Client error: ";

            if (error == QOpcUaClient::ClientError::AccessDenied) {
                errorMessage += "Got Access denied";
            } else if (error == QOpcUaClient::ClientError::ConnectionError) {
                errorMessage += "Got Connection error";
// Qt 5 has different internal error mappings...
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            } else if (error == QOpcUaClient::ClientError::InvalidAuthenticationInformation) {
                errorMessage += "Authentication information is invalid";
            } else if (error == QOpcUaClient::ClientError::NoMatchingUserIdentityTokenFound) {
                errorMessage += "No matching authentication information found";
#endif
            } else {
                errorMessage += QString::number(static_cast<int>(error));
            }

            errorMessage += " for: " + m_currentEndpointDescription.endpointUrl();
            VERBOSELOG(errorMessage);
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

QString OpcUaCore::defaultPkiPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/pki";
}

void OpcUaCore::clearPkiConfig()
{
    const QString pkiPath = defaultPkiPath();
    if (QDir().exists(pkiPath)) {
        if (!QDir(pkiPath).removeRecursively()) {
            VERBOSELOG(
                "Failed to delete files for resetting PKI config, please check and unlock/delete "
                << pkiPath << ". After that, restart caQtDM.");
        }
    }
}

void OpcUaCore::setupPkiConfig()
{
    const QString pkiPath = defaultPkiPath();
    const QString certFileName(pkiPath + "/own/certs/caQtDM.der");
    const QString privateKeyFileName(pkiPath + "/own/private/caQtDM.pem");

    const bool createCertificate = !QFile::exists(certFileName)
                                   || !QFile::exists(privateKeyFileName);
    if (createCertificate && !X509Certificate::createCertificate(pkiPath)) {
        VERBOSELOG("Could not set up directory" << pkiPath);
    }

    QOpcUaPkiConfiguration pkiConfig;

    pkiConfig.setClientCertificateFile(certFileName);
    pkiConfig.setPrivateKeyFile(privateKeyFileName);
    pkiConfig.setTrustListDirectory(pkiPath + "/trusted/certs");
    pkiConfig.setRevocationListDirectory(pkiPath + "/trusted/crl");
    pkiConfig.setIssuerListDirectory(pkiPath + "/issuers/certs");
    pkiConfig.setIssuerRevocationListDirectory(pkiPath + "/issuers/crl");

    const QStringList toCreate = {pkiConfig.trustListDirectory(),
                                  pkiConfig.revocationListDirectory(),
                                  pkiConfig.issuerListDirectory(),
                                  pkiConfig.issuerRevocationListDirectory()};
    for (const QString &dir : toCreate) {
        if (!QDir().mkpath(dir)) {
            VERBOSELOG("Could not create directory" << dir);
        }
    }

    m_client->setPkiConfiguration(pkiConfig);
}

QOpcUaEndpointDescription OpcUaCore::getEndpointWithLowestLatency(
    const QVector<QOpcUaEndpointDescription> &endpointDescriptions)
{
    QOpcUaEndpointDescription chosenEndpoint;
    chosenEndpoint.setEndpointUrl("");

    if (endpointDescriptions.isEmpty()) {
        return chosenEndpoint;
    }

    QList<QTcpSocket *> sockets;
    std::atomic<bool> found(false);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QString timeoutString = qgetenv("CAQTDM_OPCUA_MAX_LATENCY");
    int timeout;
    {
        bool ok = false;
        timeout = timeoutString.toInt(&ok);
        if (!ok) {
            timeout = DEFAULT_MAX_LATENCY;
        }
    }

    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    // For each endpoint returned from the server, try to establish a simple tcp connection. The first endpoint that connects is chosen for further opcua communication.
    for (int i = 0; i < endpointDescriptions.size(); ++i) {
        QOpcUaEndpointDescription ep = endpointDescriptions.at(i);
        QUrl url = ep.endpointUrl();
        QTcpSocket *sock = new QTcpSocket(this);
        sockets.append(sock);

        QObject::connect(sock, &QTcpSocket::connected, this, [&, ep]() {
            if (found.exchange(true))
                return;
            chosenEndpoint = ep;
            timer.stop();
            loop.quit();
            for (QTcpSocket *s : sockets) {
                if (s) {
                    if (s->state() == QAbstractSocket::ConnectedState) {
                        s->abort();
                    }
                    s->deleteLater();
                }
            }
        });
        ;
        sock->connectToHost(url.host(), url.port());
    }

    // Try to connect to all endpoints for a certain time. Due to signal / slot mechanism, the fastest connection will usually be chosen.
    timer.start(timeout);
    loop.exec();

    return chosenEndpoint;
}

QOpcUaEndpointDescription OpcUaCore::chooseEndpointDescription(
    const QVector<QOpcUaEndpointDescription> &endpointDescriptions, const QUrl &fallbackUrl)
{
    QVector<QOpcUaEndpointDescription> certificateEndpoints;
    QVector<QOpcUaEndpointDescription> usernamePasswordEndpoints;
    QVector<QOpcUaEndpointDescription> anonymousEndpoints;

    bool isCertificateSupported = m_client->authenticationInformation().authenticationType()
                                      == QOpcUaUserTokenPolicy::Certificate
                                  && m_client->pkiConfiguration().isPkiValid();
    bool isUsernamePasswordSupported = m_client->authenticationInformation().authenticationType()
                                       == QOpcUaUserTokenPolicy::Username;

    // Get all supported endpoints
    for (auto ep : endpointDescriptions) {
        if ((!isCertificateSupported
             && ep.securityPolicy()
                    == QStringLiteral("http://opcfoundation.org/UA/SecurityPolicy#None"))
            || isCertificateSupported) {
            if (ep.userIdentityTokensRef().isEmpty()) {
                // No tokens specified -> no username / password auth supported
                anonymousEndpoints.push_back(ep);
                break;
            }
            for (QOpcUaUserTokenPolicy &token : ep.userIdentityTokens()) {
                if (isCertificateSupported
                    && token.tokenType() == QOpcUaUserTokenPolicy::Certificate) {
                    certificateEndpoints.push_back(ep);
                } else if (isUsernamePasswordSupported
                           && token.tokenType() == QOpcUaUserTokenPolicy::Username) {
                    usernamePasswordEndpoints.push_back(ep);
                } else if (token.tokenType() == QOpcUaUserTokenPolicy::Anonymous) {
                    anonymousEndpoints.push_back(ep);
                }
            }
        }
    }

    QOpcUaEndpointDescription chosenEndpoint;
    chosenEndpoint.setEndpointUrl("");
    if (certificateEndpoints.isEmpty() && usernamePasswordEndpoints.isEmpty()
        && anonymousEndpoints.isEmpty()) {
        return chosenEndpoint;
    }

    // in case any of the groups don't include the fallback url, clone the first of them with it as the endpointUrl
    for (QVector<QOpcUaEndpointDescription> *endpointList :
         {&certificateEndpoints, &usernamePasswordEndpoints, &anonymousEndpoints}) {
        if (!endpointList->isEmpty()
            && std::any_of(endpointList->constBegin(),
                           endpointList->constEnd(),
                           [&fallbackUrl](const QOpcUaEndpointDescription &ep) {
                               return ep.endpointUrl() == fallbackUrl.toString();
                           })) {
            QOpcUaEndpointDescription cloneWithFallbackUrl = endpointList->first();
            cloneWithFallbackUrl.setEndpointUrl(fallbackUrl.toString());
            endpointList->append(cloneWithFallbackUrl);
        }
    }

    // check if any certificate endpoints are reachable
    chosenEndpoint = getEndpointWithLowestLatency(certificateEndpoints);
    if (!chosenEndpoint.endpointUrl().isEmpty()) {
        return chosenEndpoint;
    }
    // check if any username / password endpoints are reachable
    chosenEndpoint = getEndpointWithLowestLatency(usernamePasswordEndpoints);
    if (!chosenEndpoint.endpointUrl().isEmpty()) {
        return chosenEndpoint;
    }
    // check if any anonymous endpoints are reachable
    chosenEndpoint = getEndpointWithLowestLatency(anonymousEndpoints);
    if (!chosenEndpoint.endpointUrl().isEmpty()) {
        return chosenEndpoint;
    }

    // Since we didn't find anything, we return an invalid chosenEndpoint (empty endpointUrl)
    return chosenEndpoint;
}

bool OpcUaCore::connectOpc(const QString &url)
{
    if (!m_client) {
        VERBOSELOG("Client is not initialized.");
        return false;
    }
    m_latestEndpointUrl = url;

    auto conn = new QMetaObject::Connection;
    *conn = QObject::connect(
        m_client,
        &QOpcUaClient::endpointsRequestFinished,
        this,
        [this, conn](const QVector<QOpcUaEndpointDescription> &returnedEndpoints,
                     QOpcUa::UaStatusCode status,
                     const QUrl &url) {
            QObject::disconnect(*conn);
            delete conn;

            // If no endpoints are returned at all, there is something fundamentally wrong with the server.
            // Thus, not even the fallbackEndpoint is checked from the pv, and we error out here.
            if (returnedEndpoints.isEmpty()) {
                VERBOSELOG("No endpoints received.");
                return;
            }

            if (status != QOpcUa::UaStatusCode::Good) {
                VERBOSELOG("Received status not good: " << status);
                return;
            }

            QOpcUaEndpointDescription chosenEndpoint = chooseEndpointDescription(returnedEndpoints,
                                                                                 url);

            if (chosenEndpoint.endpointUrl().isEmpty()) {
                VERBOSELOG("No reachable endpoint hosts.");
                return;
            }

            m_client->connectToEndpoint(chosenEndpoint);
            m_currentEndpointDescription = chosenEndpoint;
        });

    m_client->requestEndpoints(url);
    return true;
}

void OpcUaCore::disconnectOpc()
{
    if (m_client
        && (m_client->state() == QOpcUaClient::ClientState::Connected
            || m_client->state() == QOpcUaClient::ClientState::Connecting)) {
        VERBOSELOG("Disconnecting from OPC UA Server....");
        m_client->disconnectFromEndpoint();
    } else {
        VERBOSELOG("Client not connected or already disconnected.");
    }
    m_currentEndpointDescription.setEndpointUrl("");
}

void OpcUaCore::subscribeToNode(const SubscriptionSettings &subscriptionSettings)
{
    QString nodeId = subscriptionSettings.nodeid;
    int intervalMs = subscriptionSettings.samplingIntervalMs;

    if (!isClientConnected()) {
        VERBOSELOG("Client is not connected.");
        return;
    }

    if (m_subscriptionNodes.contains(nodeId)) {
        return;
    }

    QOpcUaNode *node = m_client->node(nodeId);
    if (!node) {
        VERBOSELOG("Failed to create node object for subscription: " << nodeId);
        return;
    }
    m_subscriptionNodes.insert(nodeId, node);

    m_intervalMsForNodeId[nodeId] = intervalMs;

    startMonitoringOfNode(node);
}

void OpcUaCore::startMonitoringOfNode(QOpcUaNode *node)
{
    QString nodeId = node->nodeId();
    if (m_isConnectingToNode[nodeId]) {
        return;
    }

    m_isConnectingToNode[nodeId] = true;
    int intervalMs = m_intervalMsForNodeId.value(nodeId, 10);

    auto conn = new QMetaObject::Connection;
    *conn
        = QObject::connect(node, &QOpcUaNode::attributeRead, this, [=](QOpcUa::NodeAttributes attrs) {
              QObject::disconnect(*conn);
              delete conn;
              // Check for value errors
              QOpcUa::UaStatusCode statusCode = node->valueAttributeError();
              if (statusCode && statusCode != QOpcUa::UaStatusCode::Good) {
                  emit attributeGotError(nodeId,
                                         QString::fromUtf8(
                                             QMetaEnum::fromType<QOpcUa::UaStatusCode>().valueToKey(
                                                 statusCode)));
              }

              if (!attrs.testFlag(QOpcUa::NodeAttribute::NodeClass)) {
                  VERBOSELOG("Failed to read NodeClass for node: " << nodeId);
                  node->deleteLater();
                  m_subscriptionNodes.remove(nodeId);
                  m_isConnectingToNode[nodeId] = false;
                  return;
              }

              auto nodeClass = static_cast<QOpcUa::NodeClass>(
                  node->attribute(QOpcUa::NodeAttribute::NodeClass).toInt());
              if (nodeClass != QOpcUa::NodeClass::Variable) {
                  VERBOSELOG("Node " << nodeId << " is not a Variable. Subscription aborted.");
                  node->deleteLater();
                  m_subscriptionNodes.remove(nodeId);
                  m_isConnectingToNode[nodeId] = false;
                  return;
              }

              // Enable monitoring
              QOpcUaMonitoringParameters params;
              params.setSamplingInterval(intervalMs);
              params.setMonitoringMode(QOpcUaMonitoringParameters::MonitoringMode::Reporting);
              params.setSubscriptionType(QOpcUaMonitoringParameters::SubscriptionType::Shared);

              if (!node->enableMonitoring(QOpcUa::NodeAttribute::Value, params)) {
                  VERBOSELOG("Failed to enable monitoring for node: " << nodeId);
                  node->deleteLater();
                  m_subscriptionNodes.remove(nodeId);
                  m_isConnectingToNode[nodeId] = false;
                  return;
              }

              QObject::connect(node,
                               &QOpcUaNode::dataChangeOccurred,
                               this,
                               [this, nodeId](QOpcUa::NodeAttribute attr, const QVariant &value) {
                                   if (attr == QOpcUa::NodeAttribute::Value) {
                                       if (value.isValid()) {
                                           emit valueRead(nodeId, value);
                                       } else {
                                           emit attributeGotError(nodeId, "Invalid Value");
                                       }
                                   }
                               });

              VERBOSELOG("Subscribed successfully to:" << nodeId);

              QVariant accessLevel = node->attribute(QOpcUa::NodeAttribute::UserAccessLevel);

              if (accessLevel.isValid()) {
                  bool readAccess = accessLevel.value<quint8>()
                                    & static_cast<quint8>(QOpcUa::AccessLevelBit::CurrentRead);
                  bool writeAccess = accessLevel.value<quint8>()
                                     & static_cast<quint8>(QOpcUa::AccessLevelBit::CurrentWrite);
                  emit accessLevelRead(nodeId, readAccess, writeAccess);
              }

              m_isConnectingToNode[nodeId] = false;
          });

    node->readAttributes(QOpcUa::NodeAttribute::NodeClass | QOpcUa::NodeAttribute::UserAccessLevel
                         | QOpcUa::NodeAttribute::Value | QOpcUa::NodeAttribute::Description);
}

void OpcUaCore::clearAllSubscriptions()
{
    for (auto it = m_subscriptionNodes.begin(); it != m_subscriptionNodes.end(); ++it) {
        QOpcUaNode *node = it.value();
        if (node) {
            node->disableMonitoring(QOpcUa::NodeAttribute::Value);
            node->disconnect();
            unsubscribeFromNode(node->nodeId());
        }
    }

    m_subscriptionNodes.clear();
    VERBOSELOG("All OPC UA subscriptions have been cleared.");
}

bool OpcUaCore::isClientConnected()
{
    if (!m_client || m_client->state() != QOpcUaClient::Connected) {
        return false;
    }
    return true;
}

bool OpcUaCore::hasSubscription(const QString &nodeId) const
{
    return m_subscriptionNodes.contains(nodeId);
}

void OpcUaCore::unsubscribeFromNode(const QString &nodeId)
{
    if (!m_subscriptionNodes.contains(nodeId))
        return;

    m_intervalMsForNodeId.remove(nodeId);
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    m_subscriptionNodes.remove(nodeId);

    if (node) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
        node->deleteLater();
    }
}

void OpcUaCore::disableMonitoringForNode(const QString &nodeId)
{
    if (!m_subscriptionNodes.contains(nodeId))
        return;
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (node) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QT_VARIANT_TYPE(value) value.typeId()
#else
#define QT_VARIANT_TYPE(value) value.type()
#endif

bool OpcUaCore::writeDataDynamically(QOpcUaNode *node,
                                     std::function<QVariant(const QVariant &)> makeValue)
{
    auto doWrite = [&](const QVariant &ref) {
        QVariant valueToWrite = makeValue(ref);
        if (!valueToWrite.isValid()) {
            VERBOSELOG("Unsupported type");
            return;
        }

        auto conn = new QMetaObject::Connection;
        *conn = QObject::connect(node,
                                 &QOpcUaNode::attributeWritten,
                                 this,
                                 [=](QOpcUa::NodeAttribute attr, QOpcUa::UaStatusCode status) {
                                     QObject::disconnect(*conn);
                                     delete conn;
                                     if (attr == QOpcUa::NodeAttribute::Value
                                         && status != QOpcUa::Good) {
                                         VERBOSELOG(
                                             "Write failed: " << QOpcUa::statusToString(status));
                                     }
                                 });

        node->writeValueAttribute(valueToWrite);
    };

    QVariant existingValue = node->attribute(QOpcUa::NodeAttribute::Value);
    if (existingValue.isValid()) {
        doWrite(existingValue);
        return true;
    }

    auto conn = new QMetaObject::Connection;
    *conn = QObject::connect(node,
                             &QOpcUaNode::attributeRead,
                             this,
                             [=](QOpcUa::NodeAttributes attrs) {
                                 QObject::disconnect(*conn);
                                 delete conn;
                                 if (!attrs.testFlag(QOpcUa::NodeAttribute::Value)) {
                                     VERBOSELOG("Value not readable");
                                     return;
                                 }
                                 QVariant existingValue = node->attribute(
                                     QOpcUa::NodeAttribute::Value);
                                 if (existingValue.isValid()) {
                                     doWrite(existingValue);
                                 } else {
                                     emit attributeGotError(node->nodeId(), "Invalid Value");
                                 }
                             });

    node->readValueAttribute();
    return true;
}

bool OpcUaCore::writeValue(
    const QString &nodeId, double rdata, int32_t idata, char *sdata, char *errmess)
{
    if (!m_subscriptionNodes.contains(nodeId)) {
        VERBOSELOG("Node not found");
        qstrcpy(errmess, "Node not found");
        return false;
    }

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        VERBOSELOG("Node is null");
        qstrcpy(errmess, "Node is null");
        return false;
    }

    auto makeValue = [=](const QVariant &ref) -> QVariant {
        switch (QT_VARIANT_TYPE(ref)) {
        case QMetaType::Double:
        case QMetaType::Float:
            return QVariant::fromValue<double>(rdata);
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Long:
        case QMetaType::ULong:
            return QVariant::fromValue<int32_t>(idata);
        case QMetaType::Short:
            return QVariant::fromValue<int16_t>(idata);
        case QMetaType::Bool:
            return QVariant::fromValue<bool>(idata != 0);
        case QMetaType::QString:
            return QString::fromUtf8(sdata ? sdata : "");
        default:
            return {};
        }
    };

    return writeDataDynamically(node, makeValue);
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
    if (!m_subscriptionNodes.contains(nodeId)) {
        VERBOSELOG("Node not found");
        qstrcpy(errmess, "Node not found");
        return false;
    }

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        VERBOSELOG("Node is null");
        qstrcpy(errmess, "Node is null");
        return false;
    }

    auto makeValue = [=](const QVariant &ref) -> QList<QVariant> {
        QList<QVariant> values;

        if (!ref.canConvert<QVariantList>()) {
            VERBOSELOG(
                "Tried writing array data to a variable that didn't return an array last time");
            qstrcpy(errmess,
                    "Tried writing array data to a variable that didn't return an array last time");
            return values;
        }

        values.reserve(nelm);

        switch (QT_VARIANT_TYPE(ref.toList().first())) {
        case QMetaType::Double:
            for (int i = 0; i < nelm; ++i)
                values.append(ddata[i]);
            break;
        case QMetaType::Float:
            for (int i = 0; i < nelm; ++i)
                values.append(fdata[i]);
            break;
        case QMetaType::Int:
        case QMetaType::UInt:
        case QMetaType::LongLong:
        case QMetaType::ULongLong:
        case QMetaType::Long:
        case QMetaType::ULong:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<int32_t>(data32[i]));
            break;
        case QMetaType::Short:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<int16_t>(data16[i]));
            break;
        case QMetaType::Bool:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<bool>(data16[i] != 0));
            break;
        case QMetaType::QString:
            values.append(QString::fromUtf8(sdata ? sdata : ""));
            break;
        default:
            break;
        }
        return values;
    };

    return writeDataDynamically(node, makeValue);
}

QString OpcUaCore::getDescription(const QString &nodeId)
{
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        VERBOSELOG("Node is null");
        return "Node is null";
    }
    return node->attribute(QOpcUa::NodeAttribute::Description).value<QOpcUaLocalizedText>().text();
}

QString OpcUaCore::getTimestamp(const QString &nodeId)
{
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        VERBOSELOG("Node is null");
        return "Node is null";
    }
    QDateTime timestamp = node->serverTimestamp(QOpcUa::NodeAttribute::Value);
    return "Timestamp: " + timestamp.toString("MMM dd, yyyy HH:mm:ss.zzz");
}

void OpcUaCore::updatePasswordCredentials(const PasswordCredentials &newPasswordCredentials)
{
    QOpcUaAuthenticationInformation authInfo;
    authInfo.setUsernameAuthentication(newPasswordCredentials.username,
                                       newPasswordCredentials.password);
    m_client->setAuthenticationInformation(authInfo);
    if (!m_currentEndpointDescription.endpointUrl().isEmpty()) {
        m_client->connectToEndpoint(m_currentEndpointDescription);
    } else {
        connectOpc(m_latestEndpointUrl.toString());
    }
    m_passwordCredentials = newPasswordCredentials;
}

void OpcUaCore::setPemPassword(const QString &newPassword)
{
    m_pemPassword = newPassword;
}
