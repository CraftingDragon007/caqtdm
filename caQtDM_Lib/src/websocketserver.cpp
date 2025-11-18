#include "caqtdm_lib.h"
#include "websocketserver.h"
#include <QDebug>
#include <QFileInfo>
#include <QHostAddress>

WebSocketServer::WebSocketServer(QObject *parent) : QObject(parent), m_isInitialized(false)
{}

WebSocketServer& WebSocketServer::instance() {
    static WebSocketServer wsServer;
    return wsServer;
}

bool WebSocketServer::isInitialized() const {
    return this->m_isInitialized;
}

bool WebSocketServer::setup(quint16 port) {
    m_pWebSocketServer = new QWebSocketServer(QStringLiteral("caQtDM Websocket Server"),
                                              QWebSocketServer::NonSecureMode, this);

    if (m_pWebSocketServer->listen(QHostAddress::LocalHost, port)) {
        qDebug() << "WebSocketServer listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebSocketServer::onNewConnection);
        m_isInitialized = true;
        return true;
    } else {
        qCritical() << "Failed to start WebSocket server on port" << port << ":" << m_pWebSocketServer->errorString();
        return false;
    }
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

    qDebug() << "New connection from:" << pSocket->peerAddress().toString() << ":" << pSocket->peerPort();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    {
        QWriteLocker locker(&m_clientReadWriteLock);
        m_clients << pSocket;
    }
}

void WebSocketServer::processTextMessage(const QString &message)
{
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
    qDebug() << "Text message received from" << pSender->peerAddress().toString() << ":" << pSender->peerPort() << ":" << message;

    if (pSender) {
        if (message.startsWith("RESOLVE|")) {
            QStringList items = message.split('|');
            QString file;
            QString macros;
            if (items.length() == 2) {
                file = items[1];
            } else if (items.length() == 3) {
                file = items[1];
                macros = items[2].replace(':', '=').replace(',', ';');
            } else {
                pSender->sendTextMessage("ERROR|Invalid amount of arguments received, should either be 2 or 3");
                return;
            }

            QString key = file;
            if (!macros.isEmpty()) {
                key += '\0';
                key += macros;
            }

            quint16 vncPort = CaQtDM_Lib::vncPort + CaQtDM_Lib::vncPortIndex;
            quint16 webPort = CaQtDM_Lib::webPort + CaQtDM_Lib::vncPortIndex;
            bool incrementVncPort = false;

            {
                QWriteLocker webChildProcessesLocker(&CaQtDM_Lib::webChildProcessesLock);
                foreach (QString item, CaQtDM_Lib::webChildProcesses.keys()) {
                    QStringList splitItem = item.split('\0');
                    QFileInfo info(splitItem[0]);
                    if (key == info.fileName() + (splitItem.length() == 1 ? "" : '\0' + splitItem[1])) {
                        key = item;
                        break;
                    }
                }
                auto result = CaQtDM_Lib::webChildProcesses.find(key);
                if (result != CaQtDM_Lib::webChildProcesses.constEnd()) {
                    if (result.value() == nullptr) {
                        CaQtDM_Lib::webChildProcesses.remove(key);
                        qWarning().noquote() << QString("caQtDM_Web -- found undefined child process for file (and macros) %1, this shouldn't happen").arg(key.replace('\0', ' '));
                        incrementVncPort = true;
                    } else {
                        if (result.value()->process() == nullptr || result.value()->process()->state() == QProcess::ProcessState::NotRunning) {
                            CaQtDM_Lib::webChildProcesses.remove(key);
                            vncPort = result.value()->vncPort();
                            webPort = result.value()->webPort();
                        } else {
                            sendInstanceInfo(pSender, result.value()->vncPort());
                            return;
                        }
                    }
                } else incrementVncPort = true;
            }

            if (!QFile::exists(file)) {
                bool found = false;
                foreach (QString path , qgetenv("CAQTDM_DISPLAY_PATH").split(':')) {
                    if (QFile::exists(path + file)) {
                        found = true;
                        file = path + file;
                        break;
                    }
                }

                if (!found) {
                    pSender->sendTextMessage("ERROR|File not found");
                    return;
                }
            }

            if (incrementVncPort) {
                CaQtDM_Lib::vncPortIndex++;
            }

            VncWebChildProcess *item = CaQtDM_Lib::startVncChildProcess(vncPort, webPort, file, macros);
            {
                QWriteLocker webChildProcessesLocker(&CaQtDM_Lib::webChildProcessesLock);
                CaQtDM_Lib::webChildProcesses.insert(key, item);
            }
            sendInstanceInfo(pSender, vncPort);
        }
    }
}

void WebSocketServer::sendInstanceInfo(QWebSocket *receiver, quint16 vncPort) {
    if (receiver == nullptr) return;

    QString pathStr = QString::number(vncPort % 100);
    receiver->sendTextMessage("INSTANCE|" + pathStr);
}

void WebSocketServer::processBinaryMessage(const QByteArray &message)
{
    QWebSocket *pSender = qobject_cast<QWebSocket *>(sender());
    qDebug() << "Binary message received from" << pSender->peerAddress().toString() << ":" << pSender->peerPort() << ":" << message.size() << "bytes";

    if (pSender) {
        pSender->sendBinaryMessage("Server received your binary data");
    }
}

void WebSocketServer::socketDisconnected()
{
    QWebSocket *pSocket = qobject_cast<QWebSocket *>(sender());
    if (pSocket) {
        qDebug() << "Client disconnected:" << pSocket->peerAddress().toString() << ":" << pSocket->peerPort();
        {
            QWriteLocker locker(&m_clientReadWriteLock);
            m_clients.removeAll(pSocket);
        }
        pSocket->deleteLater();
    }
}

void WebSocketServer::sendLog(const QString text) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("LOG|" + text);
        }
    }
}

void WebSocketServer::sendOpenFileRequest(const QString file, const QString macros, quint16 vncPort) {
    QFileInfo fileInfo(file);
    QString fileName = fileInfo.fileName();
    QString pathStr = QString::number(vncPort % 100);
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("OPEN|" + pathStr + '|' + fileName + '|' + macros);
        }
    }
}
