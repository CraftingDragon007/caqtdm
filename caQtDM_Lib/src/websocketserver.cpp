#include "caqtdm_lib.h"
#include "websocketserver.h"
#include <QDebug>
#include <QFileInfo>
#include <QHostAddress>
#include <fileFunctions.h>
#include "webportpool.h"

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
        tryScheduleTimeout(0); // Maybe first user leaves immediately
        m_isInitialized = true;
        return true;
    } else {
        qCritical() << "Failed to start WebSocket server on port" << port << ":" << m_pWebSocketServer->errorString();
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

    qDebug().noquote() << "New connection from:" << pSocket->peerAddress().toString() + ":" + QString::number(pSocket->peerPort()) << "(" + getIPAddress(pSocket) + ")";

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
    qDebug().noquote() << "Text message received from" << pSender->peerAddress().toString() + ":" + QString::number(pSender->peerPort()) << "(" + getIPAddress(pSender) + ")" << ":" << message;

    if (pSender) {
        if (message.startsWith("PING")) {
            pSender->sendTextMessage("PONG");
        }else if (message.startsWith("RESOLVE|") && !CaQtDM_Lib::slaveServer) {
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

            QStringList keyParts = key.split('\0');
            QString macrosString;
            if (keyParts.length() > 1) {
                macrosString = keyParts[1];
            }

            VncWebChildProcess* process = CaQtDM_Lib::getWebChildProcess(file, macrosString);

            if (process) {
                sendInstanceInfo(pSender, process->vncPort(), process->webPort());
                return;
            }

            quint16 vncPort;
            quint16 webPort;

            if (!WebPortPool::instance()->allocate(vncPort, webPort)) {
                qWarning() << "Failed to allocate ports for new instance" << file << "- pool exhausted ("
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
    qDebug().noquote() << "Binary message received from" << pSender->peerAddress().toString() + ":" + QString::number(pSender->peerPort()) << "(" + getIPAddress(pSender) + ")" << ":" << message.size() << "bytes";

    if (pSender) {
        pSender->sendBinaryMessage("Server received your binary data");
    }
}

void WebSocketServer::socketDisconnected()
{
    QWebSocket *pSocket = qobject_cast<QWebSocket *>(sender());
    if (pSocket) {
        qDebug().noquote() << "Client disconnected:" << pSocket->peerAddress().toString() + ":" + QString::number(pSocket->peerPort()) << "(" + getIPAddress(pSocket) + ")";
        int count;
        {
            QWriteLocker locker(&m_clientReadWriteLock);
            m_clients.removeAll(pSocket);
            count = m_clients.count();
        }
        pSocket->deleteLater();

        tryScheduleTimeout(count);
    }
}

void WebSocketServer::tryScheduleTimeout(int count) {
    if (count == 0 && CaQtDM_Lib::slaveServer && !CaQtDM_Lib::interactionBasedTimeout) {
        uint timeout = CaQtDM_Lib::webTimeout;
        if (timeout == 0) timeout = 30 * 60;
        uint timeoutMsec = timeout * 1000;
        QTimer::singleShot(timeoutMsec, this, SLOT(shutdownNoUserTimeout()));
    }
}

void WebSocketServer::shutdownNoUserTimeout() {
    QReadLocker locker(&m_clientReadWriteLock);
    if (m_clients.count() > 0) return;
    QCoreApplication::quit();
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
    QString pathStr = QString::number(vncPort % 100);
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("OPEN|" + file + '|' + macros);
        }
    }
}

void WebSocketServer::sendOpenURLRequest(const QString url) {
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
        }
    }
}

void WebSocketServer::sendProgressUpdate(int progress) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != nullptr) {
            pSocket->sendTextMessage(QString("PROGRESS|%1").arg(QString::number(progress)));
        }
    }
}
