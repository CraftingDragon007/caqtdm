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
    bool setup(const QString &caQtDM_Version, quint16 port);
    bool setup(const QString &caQtDM_Version, QString host, quint16 port);
    bool isInitialized() const;
    void sendLog(const QString &text);
    void sendOpenFileRequest(const QString &file, const QString &macros);
    void sendOpenURLRequest(const QString &url);
    void sendInstanceInfo(QWebSocket *receiver, quint16 vncPort, quint16 webPort);

    void sendInteractionBasedShutdownMsg();
    void sendUserCountUpdate(int count);

    void sendLauncherInfo(QWebSocket *receiver);

    void sendProgressInfo(int initialProgress, int maxProgress);
    void sendProgressUpdate(int progress);

    void sendError(QString message);

public slots:
    void applicationShutdown();

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
    bool m_isShuttingDown;
    QString m_caQtDM_VersionString;

    void tryScheduleTimeout(int count);
    QString getIPAddress(QWebSocket *client);

    void sendLauncherInfo(QWebSocket *receiver, QJsonValue launcherInfo);

    void sendVersionInfo(QWebSocket *receiver, const QString &version);

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

signals:
};

#endif // WEBSOCKETSERVER_H
