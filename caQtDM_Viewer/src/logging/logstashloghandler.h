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

    /**
     * @brief Buffers the specified log into a queue, to be asynchronously sent.
     * This function is thread-safe.
     * @param log: The log to buffer.
     */
    void handleLog(const Log &log) override;

public slots:
    /**
     * @brief Sends all currently buffered logs to logstash.
     * After triggering the network request, it waits for a second to allow it to reach,
     * however it does not explicitely wait for a response.
     * This function is thread-safe.
     */
    void flush() override;

#ifndef UNIT_TESTING
private slots:
#endif
    /**
     * @brief Sends all currently buffered logs to logstash.
     * An asynchronous callback is created to handle the response, however this only logs errors, failures are not retried.
     * This function must not be called from other threads than the object's thread.
     */
    void clearLogBuffer();

#ifdef UNIT_TESTING
public:
#else
private:
#endif
    int intFromEnv(const char *envName, const int defaultValue);
    /**
     * @brief Reads the buffer timeout from an env stored in seconds, and returns it in milliseconds.
     * @param defaultTimeoutS: The default timeeout in seconds
     * @return The final value to be used
     */
    int bufferTimeoutMsFromEnv(const int defaultTimeoutS = DEFAULT_BUFFER_TIMEOUT_S);
    int bufferSizeFromEnv(const int defaultBufferSize = DEFAULT_BUFFER_SIZE);
    /**
     * @brief Reads the logstash url from an env.
     * If the url is not a valid QUrl, the default value is returned and an error is logged.
     * @param defaultLogstashUrl: The default url
     * @return The final value to be used
     */
    QUrl logstashUrlFromEnv(const QString &defaultLogstashUrl = DEFAULT_LOGSTASH_URL);

    QUrl m_logstashUrl;
    QNetworkAccessManager *m_networkManager;
    QList<Log> m_logBuffer;
    QMutex m_logBufferMutex;
    QTimer *m_logBufferTimer;
    int m_logBufferTimeoutMs;
    int m_logBufferMaxSize;
};

#endif // LOGSTASHLOGHANDLER_H
