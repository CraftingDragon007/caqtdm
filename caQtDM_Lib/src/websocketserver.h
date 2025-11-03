#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QReadWriteLock>
#include "caQtDM_Lib_global.h"

class CAQTDM_LIBSHARED_EXPORT WebSocketServer : public QObject
{
    Q_OBJECT
public:
    static WebSocketServer& instance();
    bool setup(quint16 port);
    bool isInitialized() const;
    void sendLog(QString text);
    void sendOpenFileRequest(quint16 path);

private slots:
    void onNewConnection();
    void processTextMessage(const QString &message);
    void processBinaryMessage(const QByteArray &message);
    void socketDisconnected();

private:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    QReadWriteLock m_clientReadWriteLock;
    bool m_isInitialized;

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

signals:
};

#endif // WEBSOCKETSERVER_H
