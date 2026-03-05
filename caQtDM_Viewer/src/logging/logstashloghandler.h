#ifndef LOGSTASHLOGHANDLER_H
#define LOGSTASHLOGHANDLER_H

#include "abstractloghandler.h"

#include <QMutex>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QUrl>

#define DEFAULT_BUFFER_TIMEOUT_S 60
#define DEFAULT_BUFFER_SIZE 20

#ifdef QT_NO_SSL
#define DEFAULT_LOGSTASH_URL QSL("http://logstash03.psi.ch/loki/api/v1/push")
#else
#define DEFAULT_LOGSTASH_URL QSL("https://logstash03.psi.ch/loki/api/v1/push")
#endif

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logstashLogHandler);

class LogstashLogHandler : public QObject, public AbstractLogHandler
{
    Q_OBJECT
public:
    explicit LogstashLogHandler(QObject *parent = Q_NULLPTR);
    ~LogstashLogHandler() override;

    void handleLog(const Log &log) override;

public slots:
    void flush() override;
    void clearLogBuffer();

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    int intFromEnv(const char *envName, const int defaultValue);
    int bufferTimeoutMsFromEnv(const int defaultTimeoutS = DEFAULT_BUFFER_TIMEOUT_S);
    int bufferSizeFromEnv(const int defaultBufferSize = DEFAULT_BUFFER_SIZE);
    QUrl logstashUrlFromEnv(const QString &defaultLogstashUrl = DEFAULT_LOGSTASH_URL);

    QUrl m_backendUrl;
    QNetworkAccessManager *m_networkManager;
    QList<Log> m_logBuffer;
    QMutex m_logBufferMutex;
    QTimer *m_logBufferTimer;
    int m_logBufferTimeoutMs;
    int m_logBufferMaxSize;
};

#endif // LOGSTASHLOGHANDLER_H
