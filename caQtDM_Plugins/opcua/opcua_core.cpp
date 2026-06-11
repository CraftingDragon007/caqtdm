/*
 *  This file is part of the caQtDM Framework, it was developed in collaboration with
 *  the University of Lucerne (HSLU) as an Economy Project and the Paul Scherrer Institut.
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
 *  Copyright (c) 2026
 *
 *  Authors:
 *    Erik Schwarz - PSI
 *    Hrvat Leo - HSLU
 *    Joel Müller - HSLU
 */

#include "opcua_core.h"
#include <QApplication>
#include <QDebug>
#include <QOpcUaErrorState>
#include <QStandardPaths>
#include <QTimer>
#include "caQtDM_Plugins_global.h"
#include "qdir.h"
#include "qmetaobject.h"
#include "qopcuaauthenticationinformation.h"
#include "qtcpsocket.h"

#ifdef QT_OPCUA_X509
#include "x509certificate.h"
#endif

#include <QMutex>

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QOpcUaConnectionSettings>
#endif

#define INITIAL_RECONNECTION_TIMEOUT 100
#define RECONNECTION_TIMEOUT_FACTOR 2
#define MAX_RECONNECTION_TIMEOUT 60000

#define DEFAULT_MAX_LATENCY 500
#define DEFAULT_SESSION_TIMEOUT 3600000

#define NOPASS_PLACEHOLDER "caQtDM"

static QMutex s_resetPkiConfigMutex;

OpcUaCore::OpcUaCore(QObject *parent)
    : QObject(parent)
    , m_client(Q_NULLPTR)
    , m_pemPassword("")
    , m_passwordCredentials({"", ""})
{
    m_isCertificateDialogOpen = false;

    QOpcUaProvider provider;

    QStringList backends = provider.availableBackends();
    if (!backends.contains("open62541")) {
        emit userMessage(QtCriticalMsg, "Open62541 not found.");
        return;
    }

    m_client = provider.createClient("open62541");
    if (!m_client) {
        emit userMessage(QtCriticalMsg, "Failed to create OPC UA client instance.");
        return;
    }

    QString timeString = qgetenv("CAQTDM_OPCUA_MAX_LATENCY");
    {
        bool ok = false;
        m_maxLatency = std::chrono::milliseconds(timeString.toInt(&ok));
        if (!ok) {
            m_maxLatency = std::chrono::milliseconds(DEFAULT_MAX_LATENCY);
        }
    }

    timeString = qgetenv("CAQTDM_OPCUA_SESSION_TIMEOUT");
    {
        bool ok = false;
        m_sessionTimeout = std::chrono::milliseconds(timeString.toInt(&ok));
        if (!ok) {
            m_sessionTimeout = std::chrono::milliseconds(DEFAULT_SESSION_TIMEOUT);
        }
    }

    QString username = qgetenv("CAQTDM_OPCUA_USERNAME_PLAIN");
    QString password = qgetenv("CAQTDM_OPCUA_PASSWORD_PLAIN");
    if (!username.isEmpty() && !password.isEmpty()) {
        QOpcUaAuthenticationInformation authInfo;
        authInfo.setUsernameAuthentication(username, password);
        m_client->setAuthenticationInformation(authInfo);
        m_passwordCredentials = {username, password};
    }

    {
        QMutexLocker locker(&s_resetPkiConfigMutex);
        if (!qgetenv("CAQTDM_OPCUA_RESET_PKI_CONFIG").isEmpty()) {
            // This should only be done once.
            qunsetenv("CAQTDM_OPCUA_RESET_PKI_CONFIG");
            qCInfo(opcuaLog) << "Resetting PKI Config.";
            clearPkiConfig();
        }

        // Won't overwrite any existing, valid PKI config
        setupPkiConfig();
    }

    // Handle encrypted private keys having to be decrypted
    QObject::connect(m_client,
                     &QOpcUaClient::passwordForPrivateKeyRequired,
                     this,
                     [this](QString keyFilePath, QString *password, bool previousTryWasInvalid) {
                         Q_UNUSED(keyFilePath);
                         QMutexLocker locker(&m_mutex);
                         // Skipped the first time
                         if (previousTryWasInvalid) {
                             if (*password != NOPASS_PLACEHOLDER) {
                                 // Maybe the user specified a password but this pki config was created without one
                                 qCWarning(opcuaLog)
                                     << "Failed to decrypt private key with given password, trying "
                                        "default. To reset, specify CAQTDM_OPCUA_RESET_PKI_CONFIG.";
                                 *password = NOPASS_PLACEHOLDER;
                                 return;
                             }
                             qCWarning(opcuaLog)
                                 << "Failed to decrypt private key, have you specified a "
                                    "password when initializing it via environment variable? "
                                    "To reset, specify CAQTDM_OPCUA_RESET_PKI_CONFIG.";
                             *password = "";
                             return;
                         }

                         // Try runtime-provided PEM password
                         if (!m_pemPassword.isEmpty()) {
                             *password = m_pemPassword;
                             qCInfo(opcuaLog) << "Using explicitely provided password via "
                                                 "opcua://pem_password for decrypting pem.";
                             return;
                         }

                         // Try environment-variable-provided PEM password
                         QString pemPassword = qgetenv("CAQTDM_OPCUA_PEM_PASSWORD");
                         if (pemPassword.isEmpty()) {
                             // or fallback to default PEM password (the case if the user doesn't specify anything else)
                             pemPassword = NOPASS_PLACEHOLDER;
                         }
                         *password = pemPassword;
                     });

    // Make sure that reconnects also re-monitor all previously monitored nodes
    QObject::connect(m_client, &QOpcUaClient::connected, this, [this]() {
        QMutexLocker locker(&m_mutex);
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

    m_ignoreNextDisconnect = false;

    // Immediately reconnect upon getting disconnected, with increasing interval
    QObject::connect(m_client, &QOpcUaClient::disconnected, this, [this]() {
        QMutexLocker locker(&m_mutex);
        emit disconnected();

        if (m_ignoreNextDisconnect) {
            m_ignoreNextDisconnect = false;
            return;
        }

        if (m_reconnecting)
            return;
        m_reconnecting = true;

        // Welp, can't be monitoring anything when disconnected
        m_activelyMonitoredNodes.clear();

        m_reconnectionAttempt = 0;
        m_reconnectionTimeoutMs = INITIAL_RECONNECTION_TIMEOUT;

        QTimer *reconnectTimer = new QTimer(this);
        reconnectTimer->setSingleShot(true);

        QObject::connect(reconnectTimer, &QTimer::timeout, this, [this, reconnectTimer]() {
            QMutexLocker locker(&m_mutex);
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

            // QOpcUaConnectionSettings not available before qt 6.6
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            QOpcUaConnectionSettings settings = m_client->connectionSettings();
            settings.setSessionTimeout(m_sessionTimeout);
            settings.setConnectTimeout(
                2 * m_maxLatency); // Give it some more time, since it might be experiencing issues
            m_client->setConnectionSettings(settings);
#endif

            m_client->connectToEndpoint(m_currentEndpointDescription);
            m_reconnectionAttempt++;
            m_reconnectionTimeoutMs = qMin(
                m_reconnectionTimeoutMs * RECONNECTION_TIMEOUT_FACTOR,
                MAX_RECONNECTION_TIMEOUT); // Timeout is multiplied on each retry until some maxium timeout is reached

            reconnectTimer->start(m_reconnectionTimeoutMs);
        });

        reconnectTimer->start(0);
    });

    // Figure out what to do when server certificate is untrusted
    m_certificateTrustFailedAction = CertificateTrustFailedAction::Prompt;
    bool ignoreUntrustedCertificates = !qgetenv("CAQTDM_OPCUA_IGNORE_UNTRUSTED_CERT").isEmpty();
    bool rejectUntrustedCertificates = !qgetenv("CAQTDM_OPCUA_REJECT_UNTRUSTED_CERT").isEmpty();
    if (ignoreUntrustedCertificates) {
        m_certificateTrustFailedAction = CertificateTrustFailedAction::Ignore;
    } else if (rejectUntrustedCertificates) {
        m_certificateTrustFailedAction = CertificateTrustFailedAction::Abort;
    }

    // Make sure shutdowns reject ongoing certificate validations
#ifdef QT_OPCUA_X509
    m_certificateDialog = new CertificateDialog(Q_NULLPTR);
    QObject::connect(qApp,
                     &QCoreApplication::aboutToQuit,
                     m_certificateDialog,
                     &CertificateDialog::reject);
#endif

    // Debug all connectErrors and handle specific ones, e.g. for failed certificate trust
    QObject::connect(m_client, &QOpcUaClient::connectError, this, [&](QOpcUaErrorState *state) {
        QMutexLocker locker(&m_mutex);
        QString statusCodeString = QMetaEnum::fromType<QOpcUa::UaStatusCode>().valueToKey(
            state->errorCode());
        QString errorMessage = "connectError: 0x" + QString::number(state->errorCode(), 16) + " ["
                               + statusCodeString + "] at connection step: "
                               + QString::number(static_cast<quint64>(state->connectionStep()))
                               + ", isClientSideError: "
                               + (state->isClientSideError() ? "yes" : "no");
        qCCritical(opcuaLog) << errorMessage;

        if (state->errorCode() == QOpcUa::UaStatusCode::BadSecurityChecksFailed) {
            errorMessage
                = "This indicates your client certificate may not be trusted by the server. If "
                  "that's the case, add it to the servers trusted certificates. The client "
                  "certificate is stored under: "
                  + m_client->pkiConfiguration().clientCertificateFile();
            emit userMessage(QtCriticalMsg, errorMessage);
        }

#ifdef QT_OPCUA_X509
        if (state->errorCode() == QOpcUa::UaStatusCode::BadCertificateUntrusted
            && m_certificateTrustFailedAction == CertificateTrustFailedAction::Prompt
            && !m_certificateDialog->isVisible()) {
            // Prompt user to ignore / reject / trust unknown server certificate
            errorMessage = tr("Server certificate validation failed with error 0x%1 (%2).\nClick "
                              "'Abort' to abort the connect, or 'Ignore' to continue connecting. "
                              "Click 'Trust' to connect and remember this certificate.")
                               .arg(static_cast<ulong>(state->errorCode()), 8, 16, '0')
                               .arg(statusCodeString);
            int result = m_certificateDialog
                             ->showCertificate(errorMessage,
                                               m_currentEndpointDescription.serverCertificate(),
                                               m_client->pkiConfiguration().trustListDirectory());
            state->setIgnoreError(result == CertificateTrustFailedAction::Ignore);
            m_certificateTrustFailedAction = static_cast<CertificateTrustFailedAction>(result);
        }
#endif
    });

    // Debug all other errors with a somewhat useful description
    QObject::connect(
        m_client, &QOpcUaClient::errorChanged, this, [this](QOpcUaClient::ClientError error) {
            QMutexLocker locker(&m_mutex);
            QString errorMessage = "Client error: ";

            if (error == QOpcUaClient::ClientError::InvalidUrl) {
                errorMessage += "Url is invalid";
            } else if (error == QOpcUaClient::ClientError::AccessDenied) {
                errorMessage += "Got Access denied";
            } else if (error == QOpcUaClient::ClientError::ConnectionError) {
                errorMessage += "Got Connection error";
            } else if (error == QOpcUaClient::ClientError::UnknownError) {
                errorMessage += "Error unknown to Qt (unknown error)";
            } else if (error == QOpcUaClient::ClientError::UnsupportedAuthenticationInformation) {
                errorMessage += "Client provided unsupported authentication information";
// Qt 6.10 brought us many more error messages which can be very helpful
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
            } else if (error == QOpcUaClient::ClientError::InvalidAuthenticationInformation) {
                errorMessage += "Authentication information is invalid";
            } else if (error == QOpcUaClient::ClientError::InvalidEndpointDescription) {
                errorMessage += "Endpoint description is invalid";
            } else if (error == QOpcUaClient::ClientError::NoMatchingUserIdentityTokenFound) {
                errorMessage += "No matching authentication information found";
            } else if (error == QOpcUaClient::ClientError::UnsupportedSecurityPolicy) {
                errorMessage += "Client doesnt support security policy offered";
            } else if (error == QOpcUaClient::ClientError::InvalidPki) {
                errorMessage += "Certificate or key of PKI could not be loaded / is invalid";
            } else if (error == QOpcUaClient::ClientError::CertificateUntrusted) {
                errorMessage += "Server certificate is untrusted";
#endif
            } else {
                errorMessage += QString::number(static_cast<int>(error));
            }

            errorMessage += " for: " + m_currentEndpointDescription.endpointUrl();
            qCCritical(opcuaLog) << errorMessage;
        });

    // This prevents 'lost' connections leading to sessions staying open on the server, which can lead to denial of service if the server limits the amount of concurrent sessions.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, &OpcUaCore::disconnectOpc);
}

OpcUaCore::~OpcUaCore()
{
    QMutexLocker locker(&m_mutex);
#ifdef QT_OPCUA_X509
    m_certificateDialog->deleteLater();
#endif
    clearAllSubscriptions();
    QObject::disconnect(this);

    if (m_client) {
        QObject::disconnect(m_client);
        if (m_client->state() == QOpcUaClient::ClientState::Connected) {
            disconnectOpc();
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
            emit userMessage(
                QtCriticalMsg,
                "Failed to delete files for resetting PKI config, please check and unlock/delete "
                    + pkiPath + ". After that, restart caQtDM.");
        }
    }
}

void OpcUaCore::setupPkiConfig()
{
    const QString pkiPath = defaultPkiPath();

    QOpcUaPkiConfiguration pkiConfig;
    pkiConfig.setTrustListDirectory(pkiPath + "/trusted/certs");
    pkiConfig.setRevocationListDirectory(pkiPath + "/trusted/crl");
    pkiConfig.setIssuerListDirectory(pkiPath + "/issuers/certs");
    pkiConfig.setIssuerRevocationListDirectory(pkiPath + "/issuers/crl");

    // Set up all directories, including those for private key / certificate, even if no such key can be generated, in case the user wants to copy a custom one there.
    const QStringList toCreate = {pkiConfig.trustListDirectory(),
                                  pkiConfig.revocationListDirectory(),
                                  pkiConfig.issuerListDirectory(),
                                  pkiConfig.issuerRevocationListDirectory(),
                                  pkiPath + "/own/certs",
                                  pkiPath + "/own/private"};
    for (const QString &dir : toCreate) {
        if (!QDir().mkpath(dir)) {
            qCCritical(opcuaLog) << "Could not create directory" << dir;
        }
    }

    // Now try to create certificate and private key
    const QString certFileName(pkiPath + "/own/certs/caQtDM.der");
    const QString privateKeyFileName(pkiPath + "/own/private/caQtDM.pem");

    const bool createCertificate = !QFile::exists(certFileName)
                                   || !QFile::exists(privateKeyFileName);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt 5 has an incorrect certificate generation, which at best fails completly
    // and worst case generates a bad certificate which produces misterious runtime errors as it doesnt match the OPC UA specificiation.
    if (createCertificate) {
        qCCritical(opcuaLog)
            << "Certificate generation is not possible in Qt-5. If neccessary, create a "
               "certificate yourself using the script in "
               "caQtDM_Plugins/opcua/create_certificate.sh (see sourcecode)";
        return;
    }
#endif

#ifdef QT_OPCUA_X509
    if (createCertificate && !X509Certificate::createCertificate(pkiPath)) {
        qCCritical(opcuaLog) << "Could not create certificate at: " << pkiPath;
    }
#else
    if (createCertificate) {
        qCCritical(opcuaLog) << "Could not create certificate, no X509 capabilities";
        return;
    }
#endif

    pkiConfig.setClientCertificateFile(certFileName);
    pkiConfig.setPrivateKeyFile(privateKeyFileName);

    m_client->setPkiConfiguration(pkiConfig);
    m_client->setApplicationIdentity(pkiConfig.applicationIdentity());
}

int OpcUaCore::getValueForEndpoint(const QOpcUaEndpointDescription &description)
{
    int value = 0;
    switch (description.securityMode()) {
    case QOpcUaEndpointDescription::MessageSecurityMode::SignAndEncrypt:
        value += 30;
        break;
    case QOpcUaEndpointDescription::MessageSecurityMode::Sign:
        value += 20;
        break;
    case QOpcUaEndpointDescription::MessageSecurityMode::None:
        value += 10;
    default:
        break;
    }

    QString securityPolicy = description.securityPolicy();
    if (securityPolicy == "http://opcfoundation.org/UA/SecurityPolicy#Aes256_Sha256_RsaPss") {
        value += 9;
    } else if (securityPolicy == "http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256") {
        value += 7;
    } else if (securityPolicy
               == "http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep") {
        value += 5;
    } else if (securityPolicy == "http://opcfoundation.org/UA/SecurityPolicy#Basic256") {
        value += 3;
    } else if (securityPolicy == "http://opcfoundation.org/UA/SecurityPolicy#Basic128Rsa15") {
        value += 1;
    }

    return value;
}

QOpcUaEndpointDescription OpcUaCore::getEndpointWithHighestSecurity(
    QVector<QOpcUaEndpointDescription> &endpointDescriptions)
{
    QOpcUaEndpointDescription chosenEndpoint;
    chosenEndpoint.setEndpointUrl("");

    if (endpointDescriptions.isEmpty()) {
        return chosenEndpoint;
    }

    // Sorts endpoints according to predefined security value, ranking message security mode first, then security policy
    std::sort(endpointDescriptions.begin(),
              endpointDescriptions.end(),
              [](const auto &a, const auto &b) {
                  return getValueForEndpoint(a) > getValueForEndpoint(b);
              });

    QVector<QString> triedEndpoints;
    for (auto &description : endpointDescriptions) {
        QUrl url = description.endpointUrl();
        QString endpoint = url.host() + ":" + QString::number(url.port(4840));
        if (triedEndpoints.contains(endpoint)) {
            continue;
        }
        triedEndpoints.push_back(endpoint);

        QTcpSocket *sock = new QTcpSocket(Q_NULLPTR);

        sock->connectToHost(url.host(), url.port(4840));
        if (sock->waitForConnected(m_maxLatency.count())) {
            chosenEndpoint = description;
        }
        delete sock;

        if (!chosenEndpoint.endpointUrl().isEmpty()) {
            break;
        }
    }

    return chosenEndpoint;
}

QOpcUaEndpointDescription OpcUaCore::chooseEndpointDescription(
    const QVector<QOpcUaEndpointDescription> &endpointDescriptions, const QUrl &fallbackUrl)
{
    QVector<QOpcUaEndpointDescription> certificateEndpoints;
    QVector<QOpcUaEndpointDescription> usernamePasswordEndpoints;
    QVector<QOpcUaEndpointDescription> anonymousEndpoints;

    bool isCertificateSupported = false;
    if (!qgetenv("CAQTDM_OPCUA_ENABLE_CERTIFICATE").isEmpty()
        && m_client->pkiConfiguration().isPkiValid()) {
        isCertificateSupported = true;
    }
    bool isUsernamePasswordSupported = m_client->authenticationInformation().authenticationType()
                                       == QOpcUaUserTokenPolicy::Username;

    QStringList supportedSecurityPolicies = m_client->supportedSecurityPolicies();

    // Get all supported endpoints and sort them into different authentication methods (web token not supported)
    for (auto ep : endpointDescriptions) {
        if ((!isCertificateSupported
             && ep.securityMode() == QOpcUaEndpointDescription::MessageSecurityMode::None)
            || isCertificateSupported) {
            if (!supportedSecurityPolicies.contains(ep.securityPolicy()))
                continue;

            if (ep.userIdentityTokensRef().isEmpty()) {
                // No tokens specified -> no auth supported
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

    // Return early if no endpoints left after sorting
    QOpcUaEndpointDescription chosenEndpoint;
    chosenEndpoint.setEndpointUrl("");
    if (certificateEndpoints.isEmpty() && usernamePasswordEndpoints.isEmpty()
        && anonymousEndpoints.isEmpty()) {
        return chosenEndpoint;
    }

    // In case any of the groups don't include the fallback url, clone the first of them with it as the endpointUrl
    for (QVector<QOpcUaEndpointDescription> *endpointList :
         {&certificateEndpoints, &usernamePasswordEndpoints, &anonymousEndpoints}) {
        if (!endpointList->isEmpty()
            && !std::any_of(endpointList->constBegin(),
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
    chosenEndpoint = getEndpointWithHighestSecurity(certificateEndpoints);
    if (!chosenEndpoint.endpointUrl().isEmpty()) {
        return chosenEndpoint;
    }
    // check if any username / password endpoints are reachable
    chosenEndpoint = getEndpointWithHighestSecurity(usernamePasswordEndpoints);
    if (!chosenEndpoint.endpointUrl().isEmpty()) {
        return chosenEndpoint;
    }
    // check if any anonymous endpoints are reachable
    chosenEndpoint = getEndpointWithHighestSecurity(anonymousEndpoints);
    if (!chosenEndpoint.endpointUrl().isEmpty()) {
        return chosenEndpoint;
    }

    // Since we didn't find anything, we return an invalid chosenEndpoint (empty endpointUrl)
    return chosenEndpoint;
}

bool OpcUaCore::connectOpc(const QString &url)
{
    if (!m_client) {
        qCWarning(opcuaLog) << "Client is not initialized.";
        return false;
    }
    m_latestEndpoint = url;
    qDebug() << "available sec pol.: " << m_client->supportedSecurityPolicies();

    auto conn = new QMetaObject::Connection;
    *conn = QObject::connect(
        m_client,
        &QOpcUaClient::endpointsRequestFinished,
        this,
        [this, conn](const QVector<QOpcUaEndpointDescription> &returnedEndpoints,
                     QOpcUa::UaStatusCode status,
                     const QUrl &url) {
            QMutexLocker locker(&m_mutex);
            qDebug() << "returnedEndpoints: " << returnedEndpoints.size();
            for (auto &ep: returnedEndpoints) {
                qDebug() << "got ep: ";
                qDebug() << ep.endpointUrl();
                qDebug() << ep.securityLevel();
                qDebug() << ep.securityMode();
                qDebug() << ep.securityPolicy();
                for (auto &token: ep.userIdentityTokens()) {
                    qDebug() << token.tokenType();
                }
            }
            QObject::disconnect(*conn);
            delete conn;

            // If no endpoints are returned at all, there is something fundamentally wrong with the server.
            // Thus, not even the fallbackEndpoint is checked from the pv, and we error out here.
            if (returnedEndpoints.isEmpty()) {
                qCWarning(opcuaLog) << "No endpoints received.";
                return;
            }

            if (status != QOpcUa::UaStatusCode::Good) {
                qCWarning(opcuaLog) << "Received status not good: " << status;
                return;
            }

            QOpcUaEndpointDescription chosenEndpoint = chooseEndpointDescription(returnedEndpoints,
                                                                                 url);

            qDebug() << "chose endpoint: " << chosenEndpoint.endpointUrl();
            qDebug() << chosenEndpoint.securityLevel();
            qDebug() << chosenEndpoint.securityMode();
            qDebug() << chosenEndpoint.securityPolicy();
            for (auto &token: chosenEndpoint.userIdentityTokens()) {
                qDebug() << token.tokenType();
            }
            if (chosenEndpoint.endpointUrl().isEmpty()) {
                qCWarning(opcuaLog) << "No reachable endpoint hosts.";
                return;
            }

        // QOpcUaConnectionSettings not available before qt 6.6
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
            QOpcUaConnectionSettings settings = m_client->connectionSettings();
            settings.setSessionTimeout(m_sessionTimeout);
            settings.setConnectTimeout(
                2 * m_maxLatency); // Give it some more time, since it might be experiencing issues
            m_client->setConnectionSettings(settings);
#endif

            m_client->connectToEndpoint(chosenEndpoint);
            m_currentEndpointDescription = chosenEndpoint;
        });

    m_client->requestEndpoints(url);
    return true;
}

void OpcUaCore::disconnectOpc()
{
    QMutexLocker locker(&m_mutex);
    if (m_client && m_client->state() != QOpcUaClient::ClientState::Disconnected) {
        qCDebug(opcuaLog) << "Disconnecting from OPC UA Server....";
        // This next disconnect should not be reconnnected
        m_ignoreNextDisconnect = true;
        m_client->disconnectFromEndpoint();
    } else {
        qCDebug(opcuaLog) << "Client not connected or already disconnected.";
    }
    m_currentEndpointDescription.setEndpointUrl("");
}

void OpcUaCore::subscribeToNode(const SubscriptionSettings &subscriptionSettings)
{
    QString nodeId = subscriptionSettings.nodeid;
    int intervalMs = subscriptionSettings.samplingIntervalMs;

    if (!isClientConnected()) {
        qCWarning(opcuaLog) << "Client is not connected.";
        return;
    }

    if (m_subscriptionNodes.contains(nodeId) && m_activelyMonitoredNodes.contains(nodeId)) {
        // In case we already have it and there is a value available, emit it in case the old subscription was lost
        if (m_subscriptionNodes[nodeId]->valueAttribute().isValid()) {
            QString URI = m_latestEndpoint + "/" + nodeId;
            emit valueRead(URI, m_subscriptionNodes[nodeId]->valueAttribute());
        }
        return;
    }

    QOpcUaNode *node = m_client->node(nodeId);
    if (!node) {
        qCCritical(opcuaLog) << "Failed to create node object for subscription: " << nodeId;
        return;
    }
    m_subscriptionNodes.insert(nodeId, node);

    m_intervalMsForNodeId[nodeId] = intervalMs;

    startMonitoringOfNode(node);
}

void OpcUaCore::startMonitoringOfNode(QOpcUaNode *node)
{
    QString nodeId = node->nodeId();

    int intervalMs = m_intervalMsForNodeId.value(nodeId, 10);

    auto conn = new QMetaObject::Connection;
    *conn
        = QObject::connect(node, &QOpcUaNode::attributeRead, this, [=](QOpcUa::NodeAttributes attrs) {
              QMutexLocker locker(&m_mutex);

              QObject::disconnect(*conn);
              delete conn;

              if (m_activelyMonitoredNodes.contains(nodeId)) {
                  return;
              }
              m_activelyMonitoredNodes.insert(nodeId);

              QString URI = m_latestEndpoint + "/" + nodeId;

              // Check for value errors
              QOpcUa::UaStatusCode statusCode = node->valueAttributeError();
              if (statusCode && statusCode != QOpcUa::UaStatusCode::Good) {
                  emit attributeGotError(URI,
                                         QString::fromUtf8(
                                             QMetaEnum::fromType<QOpcUa::UaStatusCode>().valueToKey(
                                                 statusCode)));
              }

              if (!attrs.testFlag(QOpcUa::NodeAttribute::NodeClass)) {
                  qCCritical(opcuaLog) << "Failed to read NodeClass for node: " << nodeId;
                  node->deleteLater();
                  m_subscriptionNodes.remove(nodeId);
                  m_activelyMonitoredNodes.remove(nodeId);
                  return;
              }

              auto nodeClass = static_cast<QOpcUa::NodeClass>(
                  node->attribute(QOpcUa::NodeAttribute::NodeClass).toInt());
              if (nodeClass != QOpcUa::NodeClass::Variable) {
                  qCCritical(opcuaLog)
                      << "Node " << nodeId << " is not a Variable. Subscription aborted.";
                  node->deleteLater();
                  m_subscriptionNodes.remove(nodeId);
                  m_activelyMonitoredNodes.remove(nodeId);
                  return;
              }

              // Enable monitoring
              QOpcUaMonitoringParameters params;
              params.setSamplingInterval(intervalMs);
              params.setMonitoringMode(QOpcUaMonitoringParameters::MonitoringMode::Reporting);
              params.setSubscriptionType(QOpcUaMonitoringParameters::SubscriptionType::Shared);

              if (!node->enableMonitoring(QOpcUa::NodeAttribute::Value, params)) {
                  qCCritical(opcuaLog) << "Failed to enable monitoring for node: " << nodeId;
                  node->deleteLater();
                  m_subscriptionNodes.remove(nodeId);
                  m_activelyMonitoredNodes.remove(nodeId);
                  return;
              }

              QObject::connect(node,
                               &QOpcUaNode::dataChangeOccurred,
                               this,
                               [this, URI](QOpcUa::NodeAttribute attr, const QVariant &value) {
                                   if (attr == QOpcUa::NodeAttribute::Value) {
                                       if (value.isValid()) {
                                           emit valueRead(URI, value);
                                       } else {
                                           emit attributeGotError(URI, "Invalid Value");
                                       }
                                   }
                               });

              qCDebug(opcuaLog) << "Subscribed successfully to:" << nodeId;

              QVariant accessLevel = node->attribute(QOpcUa::NodeAttribute::UserAccessLevel);

              if (accessLevel.isValid()) {
                  bool readAccess = accessLevel.value<quint8>()
                                    & static_cast<quint8>(QOpcUa::AccessLevelBit::CurrentRead);
                  bool writeAccess = accessLevel.value<quint8>()
                                     & static_cast<quint8>(QOpcUa::AccessLevelBit::CurrentWrite);
                  emit accessLevelRead(URI, readAccess, writeAccess);
              }
          });

    node->readAttributes(QOpcUa::NodeAttribute::NodeClass | QOpcUa::NodeAttribute::UserAccessLevel
                         | QOpcUa::NodeAttribute::Value | QOpcUa::NodeAttribute::Description);
}

void OpcUaCore::disableMonitoringForNode(const QString &nodeId)
{
    m_activelyMonitoredNodes.remove(nodeId);
    if (!m_subscriptionNodes.contains(nodeId))
        return;
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (node) {
        node->disableMonitoring(QOpcUa::NodeAttribute::Value);
    }
}

void OpcUaCore::clearAllSubscriptions()
{
    const QList<QOpcUaNode *> toUnsubscribe = m_subscriptionNodes.values();
    for (auto node : toUnsubscribe) {
        if (node) {
            node->disableMonitoring(QOpcUa::NodeAttribute::Value);
            node->disconnect();
            unsubscribeFromNode(node->nodeId());
        }
    }

    m_subscriptionNodes.clear();
    qCDebug(opcuaLog) << "All OPC UA subscriptions have been cleared.";
}

bool OpcUaCore::isClientConnected() const
{
    if (!m_client || m_client->state() != QOpcUaClient::Connected) {
        return false;
    }
    return true;
}

bool OpcUaCore::hasAnySubscriptions() const
{
    return !m_subscriptionNodes.isEmpty();
}

bool OpcUaCore::hasSubscription(const QString &nodeId) const
{
    return m_subscriptionNodes.contains(nodeId);
}

void OpcUaCore::unsubscribeFromNode(const QString &nodeId)
{
    if (!m_subscriptionNodes.contains(nodeId))
        return;

    disableMonitoringForNode(nodeId);

    m_intervalMsForNodeId.remove(nodeId);
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    m_subscriptionNodes.remove(nodeId);

    if (node) {
        node->deleteLater();
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
    // Anonymous helper to execute the write operation, taking a reference value for the correct type to cast to
    // Uses the provided makeValue lambda which returns the effective value to write, casted to the type matching the reference type
    auto doWrite = [&](const QVariant &ref) {
        QVariant valueToWrite = makeValue(ref);
        if (!valueToWrite.isValid()) {
            qCCritical(opcuaLog) << "Unsupported type" << QT_VARIANT_TYPE(ref);
            return;
        }

        // Callback to report status
        auto conn = new QMetaObject::Connection;
        *conn = QObject::connect(node,
                                 &QOpcUaNode::attributeWritten,
                                 this,
                                 [=](QOpcUa::NodeAttribute attr, QOpcUa::UaStatusCode status) {
                                     QObject::disconnect(*conn);
                                     delete conn;
                                     if (attr == QOpcUa::NodeAttribute::Value
                                         && status != QOpcUa::Good) {
                                         qCCritical(opcuaLog)
                                             << "Write failed: " << QOpcUa::statusToString(status);
                                     }
                                 });

        node->writeValueAttribute(valueToWrite);
    };

    // If any existing value has already been read, take that for reference and do write
    QVariant existingValue = node->attribute(QOpcUa::NodeAttribute::Value);
    if (existingValue.isValid()) {
        doWrite(existingValue);
        return true;
    }

    // Else issue a read to get a reference attribute
    auto conn = new QMetaObject::Connection;
    *conn = QObject::connect(node,
                             &QOpcUaNode::attributeRead,
                             this,
                             [=](QOpcUa::NodeAttributes attrs) {
                                 QMutexLocker locker(&m_mutex);
                                 QObject::disconnect(*conn);
                                 delete conn;
                                 if (!attrs.testFlag(QOpcUa::NodeAttribute::Value)) {
                                     qCCritical(opcuaLog) << "Value not readable";
                                     return;
                                 }
                                 QVariant existingValue = node->attribute(
                                     QOpcUa::NodeAttribute::Value);
                                 if (existingValue.isValid()) {
                                     doWrite(existingValue);
                                 } else {
                                     QString URI = m_latestEndpoint + "/" + node->nodeId();
                                     emit attributeGotError(URI, "Invalid Value");
                                 }
                             });

    node->readValueAttribute();
    return true;
}

bool OpcUaCore::writeValue(
    const QString &nodeId, double rdata, int32_t idata, char *sdata, char *errmess)
{
    if (!m_subscriptionNodes.contains(nodeId)) {
        qCCritical(opcuaLog) << "Node not found";
        qstrcpy(errmess, "Node not found");
        return false;
    }

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        qCCritical(opcuaLog) << "Node is null";
        qstrcpy(errmess, "Node is null");
        return false;
    }

    // Anonymous helper that stores the value to write, and can be invoked to return it casted to the type specified by the reference type
    auto makeValue = [=](const QVariant &ref) -> QVariant {
        switch (QT_VARIANT_TYPE(ref)) {
        case QMetaType::Double:
        case QMetaType::Float:
            return QVariant::fromValue<double>(rdata);
        case QMetaType::Int:
        case QMetaType::LongLong:
        case QMetaType::Long:
            return QVariant::fromValue<int32_t>(idata);
        case QMetaType::ULongLong:
        case QMetaType::ULong:
        case QMetaType::UInt:
            return QVariant::fromValue<uint32_t>(static_cast<uint32_t>(idata));
        case QMetaType::Short:
            return QVariant::fromValue<int16_t>(idata);
        case QMetaType::UShort:
            return QVariant::fromValue<uint16_t>(static_cast<uint16_t>(idata));
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
        qCCritical(opcuaLog) << "Node not found";
        qstrcpy(errmess, "Node not found");
        return false;
    }

    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        qCCritical(opcuaLog) << "Node is null";
        qstrcpy(errmess, "Node is null");
        return false;
    }

    // Anonymous helper that stores the value to write, and can be invoked to return it casted to the type specified by the reference type
    auto makeValue = [=](const QVariant &ref) -> QList<QVariant> {
        QList<QVariant> values;

        if (!ref.canConvert<QVariantList>()) {
            qCCritical(opcuaLog) <<
                "Tried writing array data to a variable that didn't return an array last time";
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
        case QMetaType::LongLong:
        case QMetaType::Long:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<int32_t>(data32[i]));
            break;
        case QMetaType::UInt:
        case QMetaType::ULongLong:
        case QMetaType::ULong:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<uint32_t>(static_cast<uint32_t>(data32[i])));
            break;
        case QMetaType::Short:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<int16_t>(data16[i]));
            break;
        case QMetaType::UShort:
            for (int i = 0; i < nelm; ++i)
                values.append(QVariant::fromValue<uint16_t>(static_cast<uint16_t>(data16[i])));
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

QString OpcUaCore::getDescription(const QString &nodeId) const
{
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        qCCritical(opcuaLog) << "Node is null";
        return "Node is null";
    }
    return node->attribute(QOpcUa::NodeAttribute::Description).value<QOpcUaLocalizedText>().text();
}

QString OpcUaCore::getTimestamp(const QString &nodeId) const
{
    QOpcUaNode *node = m_subscriptionNodes[nodeId];
    if (!node) {
        qCCritical(opcuaLog) << "Node is null";
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

    // Reconnect to work with updated password credentials
    if (!m_currentEndpointDescription.endpointUrl().isEmpty()) {
        m_client->connectToEndpoint(m_currentEndpointDescription);
    } else {
        connectOpc(m_latestEndpoint);
    }
    m_passwordCredentials = newPasswordCredentials;
}

void OpcUaCore::setPemPassword(const QString &newPassword)
{
    // This is only accessed in callback with reference to m_pemPassword, so no need to trigger anything
    m_pemPassword = newPassword;
}
