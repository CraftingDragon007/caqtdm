#include "websocketserver.h"
#include <QDebug>
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
        pSender->sendTextMessage("Server received: " + message);
    }
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

void WebSocketServer::sendLog(QString text) {
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("LOG>" + text);
        }
    }
}

void WebSocketServer::sendOpenFileRequest(quint16 path) {
    QString pathStr = QString::number(path % 100);
    QReadLocker locker(&m_clientReadWriteLock);
    foreach (QWebSocket *pSocket, m_clients) {
        if (pSocket != Q_NULLPTR) {
            pSocket->sendTextMessage("OPEN>" + pathStr);
        }
    }
}
