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

#include "tst_logstashloghandler.h"

#include "logstashloghandler.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <qnetworkreply.h>

#define ENV_BUFFER_TIMEOUT "CAQTDM_LOGGING_LOGSTASH_BUFFER_TIMEOUT"
#define ENV_BUFFER_SIZE "CAQTDM_LOGGING_LOGSTASH_BUFFER_SIZE"
#define ENV_LOGSTASH_URL "CAQTDM_LOGGING_LOGSTASH_URL"

class MockNetworkReply : public QNetworkReply
{
public:
    explicit MockNetworkReply(QObject *parent = Q_NULLPTR)
        : QNetworkReply(parent) {};

    qint64 readData(char *data, qint64 maxlen) override
    {
        Q_UNUSED(data);
        Q_UNUSED(maxlen);
        return 0;
    }

    void abort() override {};
};

class MockNetworkAccessManager : public QNetworkAccessManager
{
public:
    explicit MockNetworkAccessManager(QObject *parent = Q_NULLPTR)
        : QNetworkAccessManager(parent)
    {}

    QNetworkRequest lastRequest;
    QByteArray lastPayload;

protected:
    QNetworkReply *createRequest(Operation op,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData) override
    {
        Q_UNUSED(op);
        lastRequest = request;
        if (outgoingData) {
            lastPayload = outgoingData->readAll();
        }
        return new MockNetworkReply();
    }
};

void TestLogstashLogHandler::initTestCase()
{
    // code to be executed before the first test function
}

void TestLogstashLogHandler::init()
{
    // code to be executed before each test function

    qunsetenv(ENV_BUFFER_TIMEOUT);
    qunsetenv(ENV_BUFFER_SIZE);
    qunsetenv(ENV_LOGSTASH_URL);
}

void TestLogstashLogHandler::cleanupTestCase()
{
    // code to be executed after the last test function
}

void TestLogstashLogHandler::cleanup()
{
    // code to be executed after each test function
}

void TestLogstashLogHandler::parametersInitializedFromEnv()
{
    qputenv(ENV_BUFFER_TIMEOUT, "2");
    qputenv(ENV_BUFFER_SIZE, "3");
    qputenv(ENV_LOGSTASH_URL, "http://test:123");

    LogstashLogHandler handler;

    QCOMPARE(handler.bufferTimeoutMsFromEnv(500), 2000); // env is in seconds
    QCOMPARE(handler.m_logBufferTimeoutMs, 2000);
    QCOMPARE(handler.bufferSizeFromEnv(10), 3);
    QCOMPARE(handler.m_logBufferMaxSize, 3);
    QCOMPARE(handler.logstashUrlFromEnv("http://default"), QUrl("http://test:123"));
    QCOMPARE(handler.m_logstashUrl, QUrl("http://test:123"));
}

void TestLogstashLogHandler::flushClearsBuffer()
{
    LogstashLogHandler handler;
    auto mockManager = new MockNetworkAccessManager(this);
    delete handler.m_networkManager;
    handler.m_networkManager = mockManager;

    Log log;
    log.message = "log";

    handler.handleLog(log);
    QCOMPARE(handler.m_logBuffer.size(), 1);

    // Flush should synchronously empty the buffer
    handler.flush();
    QCOMPARE(handler.m_logBuffer.size(), 0);
}

void TestLogstashLogHandler::bufferMaxSizeFlushes()
{
    LogstashLogHandler handler;
    auto mockManager = new MockNetworkAccessManager(this);
    delete handler.m_networkManager;
    handler.m_networkManager = mockManager;
    handler.m_logBufferMaxSize = 3;
    // If the timeout (60s) is reached during this test, it will break,
    // but then the performance is bad enough a fail is more than justified

    Log log1, log2, log3, log4;
    log1.message = "log1";
    log2.message = "log2";
    log3.message = "log3";
    log4.message = "log4";

    handler.handleLog(log1);
    handler.handleLog(log2);
    handler.handleLog(log3);
    QCOMPARE(handler.m_logBuffer.size(), 3);

    // Fourth log should cause a clear
    handler.handleLog(log4);
    // Wait for a second, it should not take longer than that to write async
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(1000);
    loop.exec();

    QCOMPARE(handler.m_logBuffer.size(), 0);
}

void TestLogstashLogHandler::bufferTimeoutFlushes()
{
    LogstashLogHandler handler;
    auto mockManager = new MockNetworkAccessManager();
    delete handler.m_networkManager;
    handler.m_networkManager = mockManager;
    handler.m_logBufferTimeoutMs = 100;
    handler.m_logBufferTimer->setInterval(handler.m_logBufferTimeoutMs);

    Log log;
    log.message = "message";
    handler.handleLog(log);

    // Test signal being connected properly and flushing
    QEventLoop loop;
    QTimer singleShot;
    QObject::connect(&singleShot, &QTimer::timeout, &loop, &QEventLoop::quit);
    singleShot.start(1000); // Includes potential Signal & Slot delay
    loop.exec();

    QCOMPARE(handler.m_logBuffer.size(), 0);
}

void TestLogstashLogHandler::flushSendsPayload()
{
    LogstashLogHandler handler;
    auto mockManager = new MockNetworkAccessManager(this);
    delete handler.m_networkManager;
    handler.m_networkManager = mockManager;

    Log log;
    log.message = "myMessage";

    handler.handleLog(log);
    handler.flush();

    // Mock should have received something
    QVERIFY(!mockManager->lastPayload.isEmpty());

    // Check JSON structure & contents
    QJsonDocument doc = QJsonDocument::fromJson(mockManager->lastPayload);
    QVERIFY(doc.isObject());
    QJsonObject root = doc.object();
    QVERIFY(root.contains("caqtdm_events"));
    QJsonArray events = root.value("caqtdm_events").toArray();
    QCOMPARE(events.size(), 1);
    QJsonObject firstEvent = events[0].toObject();
    QCOMPARE(firstEvent.value("message").toString(), QString("myMessage"));
}
