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
    bool setup(QString host, quint16 port);
    bool isInitialized() const;
    void sendLog(const QString text);
    void sendOpenFileRequest(const QString file, const QString macros, quint16 vncPort);
    void sendOpenURLRequest(const QString url);
    void sendInstanceInfo(QWebSocket *receiver, quint16 vncPort, quint16 webPort);

    void sendInteractionBasedShutdownMsg();
    void sendUserCountUpdate(int count);

    void sendProgressInfo(int initialProgress, int maxProgress);
    void sendProgressUpdate(int progress);
private slots:
    void onNewConnection();
    void processTextMessage(const QString &message);
    void processBinaryMessage(const QByteArray &message);
    void socketDisconnected();
    void shutdownNoUserTimeout();

private:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    QReadWriteLock m_clientReadWriteLock;
    bool m_isInitialized;

    void tryScheduleTimeout(int count);
    QString getIPAddress(QWebSocket *client);

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

signals:
};

#endif // WEBSOCKETSERVER_H
