#include "logstashloghandler.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QThread>

#ifdef QT_NO_SSL
#define DEFAULT_LOGSTASH_URL "http://logstash03.psi.ch/loki/api/v1/push"
#else
#define DEFAULT_LOGSTASH_URL "https://logstash03.psi.ch/loki/api/v1/push"
#endif

LogstashLogHandler::LogstashLogHandler(QObject *parent)
    : QObject(parent)
{
    m_logBufferTimeoutMs = 60000;
    m_logBufferMaxSize = 20;
    m_backendUrl = DEFAULT_LOGSTASH_URL;
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

        logsJsonArray.append(logJsonObject);
    }

    QJsonObject rootJsonObject;
    rootJsonObject.insert("caqtdm_events", logsJsonArray);

    QByteArray payload = QJsonDocument(rootJsonObject).toJson(QJsonDocument::Compact);

    QNetworkRequest request(m_backendUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_networkManager->post(request, payload);
}
