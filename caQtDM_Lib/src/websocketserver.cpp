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
 *  Copyright (c) 2026
 *
 *  Author:
 *    Julian Houba
 *  Contact details:
 *    julian.houba@psi.ch
 */

#include "caqtdm_lib.h"
#include "websocketserver.h"
#include <QDebug>
#include <QFileInfo>
#include <QHostAddress>
#include <fileFunctions.h>
#include "weblaunchermanager.h"
#include "webportpool.h"

Q_LOGGING_CATEGORY(webSocketServer, "caqtdm.web.webSocketServer")

WebSocketServer::WebSocketServer(QObject *parent) : QObject(parent), m_pWebSocketServer(nullptr), m_isInitialized(false), m_isShuttingDown(false)
{}

WebSocketServer& WebSocketServer::instance() {
    static WebSocketServer wsServer;
    return wsServer;
}

bool WebSocketServer::isInitialized() const {
    return this->m_isInitialized;
}

bool WebSocketServer::setup(const QString &caQtDM_Version, quint16 port) {
    return setup(caQtDM_Version, "127.0.0.1", port);
}

bool WebSocketServer::setup(const QString &caQtDM_Version, QString host, quint16 port) {
    m_caQtDM_VersionString = caQtDM_Version;
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("caQtDM Websocket Server"),
                                              QWebSocketServer::NonSecureMode, this);

    if (m_pWebSocketServer->listen(QHostAddress(host), port)) {
        qCInfo(webSocketServer) << "WebSocketServer listening on port" << port << "on host" << host;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebSocketServer::onNewConnection);
        tryScheduleTimeout(0); // Maybe first user leaves immediately
        m_isInitialized = true;
        return true;
    } else {
        qCCritical(webSocketServer) << "Failed to start WebSocket server on port" << port << "on host" << host << ":" << m_pWebSocketServer->errorString();
        return false;
    }
}

QString WebSocketServer::getIPAddress(QWebSocket *client) {
    const QNetworkRequest req = client->request();
    QByteArray xRealIp = req.rawHeader("X-Real-IP");
    if (!xRealIp.isEmpty()) {
        return QString(xRealIp).trimmed();
    }

    return client->peerAddress().toString();
}

WebSocketServer::~WebSocketServer()
{
    m_pWebSocketServer->close();
    QWriteLocker locker(&m_clientReadWriteLock);
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    if (!pSocket) return;

    if (m_isShuttingDown) {
        pSocket->close(QWebSocketProtocol::CloseCodeNormal, "Application shutdown");
        pSocket->deleteLater();
        return;
    }

    qCDebug(webSocketServer).noquote() << "New connection from:" << pSocket->peerAddress().toString() + ":" + QString::number(pSocket->peerPort()) << "(" + getIPAddress(pSocket) + ")";

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    sendVersionInfo(pSocket, m_caQtDM_VersionString);

    int count;
    {
        QWriteLocker locker(&m_clientReadWriteLock);
        m_clients << pSocket;
        count = m_clients.count();
    }

    sendUserCountUpdate(count);

    sendLauncherInfo(pSocket);

    pSocket->sendTextMessage("INTERACTIVE|" + QVariant(!CaQtDM_Lib::noVncReadonly).toString());
}

void WebSocketServer::processTextMessage(const QString &message)
{
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());

    if (pSender) {
        qCDebug(webSocketServer).noquote() << "Text message received from" << pSender->peerAddress().toString() + ":" + QString::number(pSender->peerPort()) << "(" + getIPAddress(pSender) + ")" << ":" << message;
        if (message.startsWith("PING")) {
            pSender->sendTextMessage("PONG");
        } else if (message.startsWith("RESOLVE|") && !CaQtDM_Lib::slaveServer) {
            QStringList items = message.split('|');
            QString file;
            QString macros;
            if (items.length() == 2) {
                file = items[1];
            } else if (items.length() == 3) {
                file = items[1];
                macros = items[2].replace(':', '=');
            } else {
                pSender->sendTextMessage("ERROR|Invalid amount of arguments received, should either be 2 or 3");
                return;
            }

#if defined(_MSC_VER)
            QRegularExpression driveRegex("^[A-Za-z]:(?:\\\\|/)?", QRegularExpression::CaseInsensitiveOption);
            if (file.startsWith('/') || file.startsWith('\\') || driveRegex.match(file).hasMatch()) {
#else
            if (file.startsWith('/')) {
#endif
                pSender->sendTextMessage("ERROR|No absolutes paths are allowed! Please specify a relative path.");
                return;
            }

            QString normalized = file.replace('\\', '/');
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QStringList parts = normalized.split('/', QString::SkipEmptyParts);
#else
            QStringList parts = normalized.split('/', Qt::SkipEmptyParts);
#endif
            if (parts.contains("..")) {
                pSender->sendTextMessage("ERROR|Directory traversal (../) is not allowed! Please specify a safe relative path.");
                return;
            }

            fileFunctions filefunction;
            filefunction.checkFileAndDownload(file);

            searchFile *filecheck = new searchFile(file);
            file = filecheck->findFile();
            filecheck->deleteLater();

            if (file.isNull()) {
                pSender->sendTextMessage("ERROR|File not found");
                return;
            }

            QString key = file;
            if (!macros.isEmpty()) {
                key += '\0';
                key += macros;
            }

            QStringList keyParts = key.split(QChar('\0'));
            QString macrosString;
            if (keyParts.length() > 1) {
                macrosString = keyParts[1];
            }

            VncWebChildProcess* process = CaQtDM_Lib::getWebChildProcess(file, macrosString);

            if (process) {
                sendInstanceInfo(pSender, process->vncPort(), process->webPort());
                return;
            }

            int processCount;
            {
                QReadLocker locker(&CaQtDM_Lib::webChildProcessesLock);
                processCount = CaQtDM_Lib::webChildProcesses.count();
            }

            if (processCount >= CaQtDM_Lib::webInstanceLimit - 1) {
                pSender->sendTextMessage("ERROR|Maximum instance limit reached");
                return;
            }

            quint16 vncPort;
            quint16 webPort;

            if (!WebPortPool::instance()->allocate(vncPort, webPort)) {
                qCWarning(webSocketServer) << "Failed to allocate ports for new instance" << file << "- pool exhausted ("
                           << WebPortPool::instance()->freeCount() << "free)";
                pSender->sendTextMessage("ERROR|Maximum instance limit reached");
                return;
            }

            process = CaQtDM_Lib::startVncChildProcess(vncPort, webPort, file, macros);
            {
                QWriteLocker webChildProcessesLocker(&CaQtDM_Lib::webChildProcessesLock);
                CaQtDM_Lib::webChildProcesses.insert(key, process);
            }
            sendInstanceInfo(pSender, vncPort, webPort);
        } else if (message.startsWith("SELECT_LAUNCHER|")) {
            if (!WebLauncherManager::instance().isInitialized()) {
                pSender->sendTextMessage("ERROR|Launcher selection is currently not available");
                return;
            }
            QStringList items = message.split('|');
            if (items.length() != 2) {
                pSender->sendTextMessage("ERROR|Invalid amount of arguments received, only one is needed");
                return;
            }
            QString choice = items[1];
            QJsonValue result = WebLauncherManager::instance().getLauncherFromUserChoice(choice);
            if (result.isNull()) {
                pSender->sendTextMessage(QString("ERROR|Launcher file %1 could not be found or accessed").arg(choice));
                return;
            }
            sendLauncherInfo(pSender, result);
        }
    }
}

void WebSocketServer::sendInstanceInfo(QWebSocket *receiver, quint16 vncPort, quint16 webPort) {
    if (receiver == nullptr) return;
    receiver->sendTextMessage("INSTANCE|" + QString::number(vncPort) + '|' + QString::number(webPort));
}

void WebSocketServer::processBinaryMessage(const QByteArray &message)
{
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());

    if (pSender) {
        qCDebug(webSocketServer).noquote() << "Binary message received from" << pSender->peerAddress().toString() + ":" + QString::number(pSender->peerPort()) << "(" + getIPAddress(pSender) + ")" << ":" << message.size() << "bytes";
        pSender->sendBinaryMessage("Server received your binary data");
    }
}

void WebSocketServer::socketDisconnected()
{
    if (m_isShuttingDown) return;
    QWebSocket *pSocket = qobject_cast<QWebSocket *>(sender());
    if (pSocket) {
        qCDebug(webSocketServer).noquote() << "Client disconnected:" << pSocket->peerAddress().toString() + ":" + QString::number(pSocket->peerPort()) << "(" + getIPAddress(pSocket) + ")";
        int count;
        {
            QWriteLocker locker(&m_clientReadWriteLock);
            m_clients.removeAll(pSocket);
            count = m_clients.count();
        }
        pSocket->deleteLater();

        tryScheduleTimeout(count);
        sendUserCountUpdate(count);
    }
}

void WebSocketServer::tryScheduleTimeout(int count) {
    if (count == 0 && CaQtDM_Lib::slaveServer && !CaQtDM_Lib::interactionBasedTimeout) {
        uint timeout = CaQtDM_Lib::webTimeout;
        if (timeout == 0) timeout = 30 * 60;
        uint timeoutMsec = timeout * 1000;
        QTimer::singleShot(timeoutMsec, this, SLOT(shutdownNoUserTimeout()));
        qCInfo(webSocketServer) << "Scheduled no user shutdown timeout in" << timeout << "seconds";
    }
}

void WebSocketServer::shutdownNoUserTimeout() {
    {
        QReadLocker locker(&m_clientReadWriteLock);
        qCDebug(webSocketServer) << "No user timeout reached";
        if (m_clients.count() > 0) return;
    }
    qCInfo(webSocketServer) << "No user based shutdown triggered!";
    QCoreApplication::exit(0);
}

void WebSocketServer::sendLog(const QString &text) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("LOG|" + text);
        }
    }
}

void WebSocketServer::sendOpenFileRequest(const QString &file, const QString &macros) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("OPEN|" + file + '|' + macros);
        }
    }
}

void WebSocketServer::sendOpenURLRequest(const QString &url) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("OPEN_URL|" + url);
        }
    }
}

void WebSocketServer::sendInteractionBasedShutdownMsg() {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->sendTextMessage("TIMEOUT|Interaction based timeout reached, reload the page to restart the session.");
        }
    }
}

void WebSocketServer::sendProgressInfo(int initialProgress, int maxProgress) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->sendTextMessage(QString("INIT_PROGRESS|%1|%2").arg(QString::number(initialProgress), QString::number(maxProgress)));
            pSocket->flush();
        }
    }
}

void WebSocketServer::sendProgressUpdate(int progress) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->sendTextMessage(QString("PROGRESS|%1").arg(QString::number(progress)));
            pSocket->flush();
        }
    }
}

void WebSocketServer::sendUserCountUpdate(int count) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->sendTextMessage(QString("USERS|%1").arg(QString::number(count)));
        }
    }
}


void WebSocketServer::sendLauncherInfo(QWebSocket *receiver) {
    sendLauncherInfo(receiver, WebLauncherManager::instance().getExpandedLauncherJson());
}

void WebSocketServer::sendLauncherInfo(QWebSocket *receiver, QJsonValue launcherInfo) {
    if (!launcherInfo.isNull() && receiver != nullptr && launcherInfo.isObject()) {
        QJsonDocument doc(launcherInfo.toObject());
        receiver->sendTextMessage(QString("LAUNCHER|%1").arg(QString::fromUtf8(doc.toJson(QJsonDocument::Indented))));
    }
}

void WebSocketServer::sendVersionInfo(QWebSocket *receiver, const QString& version) {
    if (receiver && !version.isEmpty()) {
        receiver->sendTextMessage(QString("VERSION|%1").arg(version));
    }
}

void WebSocketServer::sendError(QString message) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->sendTextMessage(QString("ERROR|%1").arg(message));
        }
    }
}

void WebSocketServer::applicationShutdown() {
    if (!m_isInitialized) return;
    m_isShuttingDown = true;
    QWriteLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->close(QWebSocketProtocol::CloseCodeNormal, "Application shutdown");
            m_clients.removeOne(pSocket);
            pSocket->deleteLater();
        }
    }
    m_pWebSocketServer->close();
}
