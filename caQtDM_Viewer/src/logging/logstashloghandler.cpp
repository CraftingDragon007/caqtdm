#include "logstashloghandler.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QThread>

#define ENV_BUFFER_TIMEOUT "CAQTDM_LOGGING_LOGSTASH_BUFFER_TIMEOUT"
#define ENV_BUFFER_SIZE "CAQTDM_LOGGING_LOGSTASH_BUFFER_SIZE"
#define ENV_LOGSTASH_URL "CAQTDM_LOGGING_LOGSTASH_URL"

Q_LOGGING_CATEGORY(logstashLogHandler, "logging.logstash");

LogstashLogHandler::LogstashLogHandler(QObject *parent)
    : QObject(parent)
{
    m_logBufferTimeoutMs = bufferTimeoutMsFromEnv();
    m_logBufferMaxSize = bufferSizeFromEnv();
    m_backendUrl = logstashUrlFromEnv();
    m_networkManager = new QNetworkAccessManager(this);
    m_logBufferTimer = new QTimer(this);
    QObject::connect(m_logBufferTimer,
                     &QTimer::timeout,
                     this,
                     &LogstashLogHandler::clearLogBuffer,
                     Qt::QueuedConnection);
    m_logBufferTimer->setInterval(m_logBufferTimeoutMs);
    m_logBufferTimer->start();
}

LogstashLogHandler::~LogstashLogHandler() {}

int LogstashLogHandler::bufferTimeoutMsFromEnv(int defaultTimeoutMs)
{
    const QString timeoutString = qgetenv(ENV_BUFFER_TIMEOUT);
    if (timeoutString.isEmpty()) {
        return defaultTimeoutMs;
    }

    bool ok;
    const int timeout = timeoutString.toInt(&ok); // Must be in seconds (not ms!)
    if (!ok) {
        qCWarning(logstashLogHandler)
            << ENV_BUFFER_TIMEOUT << "is set and has a value, but could not be parsed to an int";
        return defaultTimeoutMs;
    }

    return timeout * 1000;
}

int LogstashLogHandler::bufferSizeFromEnv(int defaultBufferSize)
{
    const QString bufferSizeString = qgetenv(ENV_BUFFER_SIZE);
    if (bufferSizeString.isEmpty()) {
        return defaultBufferSize;
    }

    bool ok;
    const int bufferSize = bufferSizeString.toInt(&ok);
    if (!ok) {
        qCWarning(logstashLogHandler)
            << ENV_BUFFER_SIZE << "is set and has a value, but could not be parsed to an int";
        return defaultBufferSize;
    }

    return bufferSize;
}

QUrl LogstashLogHandler::logstashUrlFromEnv(QString defaultLogstashUrl)
{
    const QString urlString = qgetenv(ENV_LOGSTASH_URL);
    if (urlString.isEmpty()) {
        return defaultLogstashUrl;
    }

    const QUrl url(urlString);
    if (!url.isValid()) {
        qCCritical(logstashLogHandler)
            << ENV_LOGSTASH_URL << "is set and has a value, but is not a valid QUrl";
        return defaultLogstashUrl;
    }

#ifdef QT_NO_SSL
    if (urlString.startsWith("https")) {
        qCWarning(logstashLogHandler)
            << ENV_LOGSTASH_URL << "is HTTPS, even though QT_NO_SSL is set. This might break.";
    }
#endif

    return url;
}

void LogstashLogHandler::handleLog(const Log &log)
{
    {
        QMutexLocker locker(&m_logBufferMutex);
        m_logBuffer.append(log);
    }

    if (m_logBuffer.size() > m_logBufferMaxSize) {
        QMetaObject::invokeMethod(this, "clearLogBuffer", Qt::QueuedConnection);
    }
}

void LogstashLogHandler::flush()
{
    if (QThread::currentThread() == this->thread()) {
        clearLogBuffer();
    } else {
        QMetaObject::invokeMethod(this, "clearLogBuffer", Qt::BlockingQueuedConnection);
    }

    // Wait for a second (while allowing current thread to do work) to let network request arrive.
    // Hardcoded to 1s to not accidentally stall forever if something goes wrong in the delivery.
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(1000);
    loop.exec();
}

void LogstashLogHandler::clearLogBuffer()
{
    QList<Log> logs;
    {
        QMutexLocker locker(&m_logBufferMutex);
        logs = m_logBuffer;
        m_logBuffer.clear();
    }

    if (logs.empty()) {
        return;
    }

    QJsonArray logsJsonArray;
    for (const Log &log : logs) {
        QJsonObject logJsonObject;
        logJsonObject.insert("timestamp", log.timestampUtc);
        logJsonObject.insert("type", static_cast<int>(log.loglevel));
        logJsonObject.insert("type_string", log.loglevelString);
        logJsonObject.insert("file", log.file);
        logJsonObject.insert("function", log.function);
        logJsonObject.insert("line", log.line);
        logJsonObject.insert("message", log.message);
        logJsonObject.insert("process_id", log.processId);
        logJsonObject.insert("category", log.category);

        logsJsonArray.append(logJsonObject);
    }

    QJsonObject rootJsonObject;
    rootJsonObject.insert("caqtdm_events", logsJsonArray);

    QByteArray payload = QJsonDocument(rootJsonObject).toJson(QJsonDocument::Compact);

    QNetworkRequest request(m_backendUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QString("caQtDM:%1/Qt:%2").arg(BUILDVERSION).arg(qVersion()));

    m_networkManager->post(request, payload);
}
