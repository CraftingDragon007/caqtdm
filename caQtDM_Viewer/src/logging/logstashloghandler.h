#ifndef LOGSTASHLOGHANDLER_H
#define LOGSTASHLOGHANDLER_H

#include "abstractloghandler.h"

#include <QMutex>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QUrl>

class LogstashLogHandler : public QObject, public AbstractLogHandler
{
    Q_OBJECT
public:
    explicit LogstashLogHandler(QObject *parent = Q_NULLPTR);
    ~LogstashLogHandler() override;

    void handleLog(const Log &log) override;

public slots:
    void flush();
    void clearLogBuffer();

private:
    QUrl m_backendUrl;
    QNetworkAccessManager *m_networkManager;
    QList<Log> m_logBuffer;
    QMutex m_logBufferMutex;
    QTimer *m_logBufferTimer;
    int m_logBufferTimeoutMs;
    int m_logBufferMaxSize;
};

#endif // LOGSTASHLOGHANDLER_H
