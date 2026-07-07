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
 *  Copyright (c) 2010 - 2026
 *
 *  Author:
 *    Erik Schwarz
 *  Contact details:
 *    erik.schwarz@psi.ch
 */

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
Q_DECLARE_LOGGING_CATEGORY(logstashLogHandlerLog)

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
    qint64 intFromEnv(const char *envName, const qint64 defaultValue);
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
