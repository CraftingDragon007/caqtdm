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
    m_logstashUrl = logstashUrlFromEnv();
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

LogstashLogHandler::~LogstashLogHandler()
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(
            this,
            [=]() {
                if (m_logBufferTimer) {
                    delete m_logBufferTimer;
                }
            },
            Qt::BlockingQueuedConnection);
    } // Otherwise, it will automatically be cleaned up by Qt due to being a child of this
}

qint64 LogstashLogHandler::intFromEnv(const char *envName, const qint64 defaultValue)
{
    const QString valueString = qgetenv(envName);
    if (valueString.isEmpty()) {
        return defaultValue;
    }

    bool ok;
    const qint64 parsedValue = valueString.toLongLong(&ok);
    if (!ok) {
        qCWarning(logstashLogHandler)
            << envName << QSL("is set and has a value, but could not be parsed to an int");
        return defaultValue;
    }

    return parsedValue;
}

int LogstashLogHandler::bufferTimeoutMsFromEnv(const int defaultTimeoutS)
{
    qint64 bufferTimeoutS = intFromEnv(ENV_BUFFER_TIMEOUT, defaultTimeoutS);
    if (bufferTimeoutS < 1) {
        qCWarning(logstashLogHandler)
            << ENV_BUFFER_TIMEOUT << QSL("specified invalid buffer timeout, must be positive");
        return defaultTimeoutS;
    }

    if (bufferTimeoutS
        > INT_MAX
              / 1000) { // Will be multiplied by 1000 afterwards to get MS, but cannot multiply in here due to potential overflow -> divide limit instead
        qCWarning(logstashLogHandler)
            << ENV_BUFFER_TIMEOUT << QSL("specified invalid buffer timeout, must be smaller than")
            << INT_MAX / 1000;
        return defaultTimeoutS;
    }

    return bufferTimeoutS * 1000;
}

int LogstashLogHandler::bufferSizeFromEnv(const int defaultBufferSize)
{
    qint64 bufferSize = intFromEnv(ENV_BUFFER_SIZE, defaultBufferSize);
    if (bufferSize < 0) {
        qCWarning(logstashLogHandler)
            << ENV_BUFFER_SIZE << QSL("specified invalid buffer size, must be >= 0");
        return defaultBufferSize;
    }

    if (bufferSize > INT_MAX) {
        qCWarning(logstashLogHandler)
            << ENV_BUFFER_SIZE << QSL("specified invalid buffer size, must be smaller than")
            << INT_MAX;
        return defaultBufferSize;
    }

    return bufferSize;
}

QUrl LogstashLogHandler::logstashUrlFromEnv(const QString &defaultLogstashUrl)
{
    const QString urlString = qgetenv(ENV_LOGSTASH_URL);
    if (urlString.isEmpty()) {
        return defaultLogstashUrl;
    }

    const QUrl url(urlString);
    if (!url.isValid()) {
        qCCritical(logstashLogHandler)
            << ENV_LOGSTASH_URL << QSL("is set and has a value, but is not a valid QUrl");
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
    bool shouldClear;
    {
        QMutexLocker locker(&m_logBufferMutex);
        m_logBuffer.append(log);
        shouldClear = m_logBuffer.size() > m_logBufferMaxSize;
    }

    if (shouldClear) {
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
        logJsonObject.insert(QSL("timestamp"), log.timestampUtc);
        logJsonObject.insert(QSL("type"), static_cast<int>(log.loglevel));
        logJsonObject.insert(QSL("type_string"), log.loglevelString);
        logJsonObject.insert(QSL("file"), log.file);
        logJsonObject.insert(QSL("function"), log.function);
        logJsonObject.insert(QSL("line"), log.line);
        logJsonObject.insert(QSL("message"), log.message);
        logJsonObject.insert(QSL("process_id"), log.processId);
        logJsonObject.insert(QSL("category"), log.category);

        logsJsonArray.append(logJsonObject);
    }

    QJsonObject rootJsonObject;
    rootJsonObject.insert(QSL("caqtdm_events"), logsJsonArray);

    const QByteArray payload = QJsonDocument(rootJsonObject).toJson(QJsonDocument::Compact);

    QNetworkRequest request(m_logstashUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QSL("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QString("caQtDM:%1/Qt:%2").arg(BUILDVERSION).arg(qVersion()));

    QNetworkReply *reply = m_networkManager->post(request, payload);
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qCCritical(logstashLogHandler)
                << QSL("Failed to send log to Logstash:") << reply->errorString();
        }
        reply->deleteLater();
    });
}
