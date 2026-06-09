#include "tst_bsread_dispatchercontrol.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "fakenetworkreply.h"
#include "zmq.h"

void Testbsread_dispatchercontrol::initTestCase()
{
    // code to be executed before the first test function

    m_dispatchercontrol = Q_NULLPTR;
}

void Testbsread_dispatchercontrol::init()
{
    // code to be executed before each test function

    m_dispatchercontrol = new bsread_dispatchercontrol();
    m_zmqContext = zmq_ctx_new();

    m_dispatchercontrol->setZmqcontex(m_zmqContext);
}

void Testbsread_dispatchercontrol::cleanupTestCase()
{
    // code to be executed after the last test function
}

void Testbsread_dispatchercontrol::cleanup()
{
    // code to be executed after each test function

    m_dispatchercontrol->initiateShutdown();
    delete m_dispatchercontrol;
    zmq_ctx_term(m_zmqContext);

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
}

void Testbsread_dispatchercontrol::finishReplyConnectWorks()
{
    m_dispatchercontrol->Channels.clear();
    m_dispatchercontrol->streams.clear();
    m_dispatchercontrol->bsreadconnections.clear();
    m_dispatchercontrol->bsreadThreads.clear();
    m_dispatchercontrol->tobeRemoved.clear();

    m_dispatchercontrol->Channels.insert("channelA", 1);
    m_dispatchercontrol->Channels.insert("channelA", 2);
    m_dispatchercontrol->Channels.insert("channelB", 3);

    QByteArray raw = R"({"configuration":{"streamType":"pub_sub"},"stream":"tcp://127.0.0.1:6000"})";
    FakeNetworkReply *reply = new FakeNetworkReply(raw, m_dispatchercontrol);

    QObject::connect(reply, SIGNAL(readyRead()), m_dispatchercontrol, SLOT(finishReplyConnect()));
    reply->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->streams.count(), 1);
    QCOMPARE(m_dispatchercontrol->streams.first(), QString("tcp://127.0.0.1:6000"));

    QCOMPARE(m_dispatchercontrol->bsreadconnections.count(), 1);
    QCOMPARE(m_dispatchercontrol->bsreadThreads.count(), 1);
    QVERIFY(m_dispatchercontrol->bsreadconnections.first());
    QCOMPARE(QString(m_dispatchercontrol->bsreadconnections.first()->getConnectionPoint()),
             QString("tcp://127.0.0.1:6000"));

    QVERIFY(m_dispatchercontrol->tobeRemoved.isEmpty());
}

void Testbsread_dispatchercontrol::finishVerificationWorks()
{
    m_dispatchercontrol->ChannelsApprovePipeline.clear();
    m_dispatchercontrol->ChannelsToBeApprovePipeline.clear();

    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_OK", 11);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_OK", 12);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:hash", 13);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_WAIT", 14);

    QJsonArray array;
    {
        QJsonObject channelObject;
        channelObject.insert("name", "CH_OK");

        QJsonObject item;
        item.insert("recording", true);
        item.insert("channel", channelObject);
        array.append(item);
    }
    {
        QJsonObject channelObject;
        channelObject.insert("name", "CH_WAIT");

        QJsonObject item;
        item.insert("recording", false);
        item.insert("channel", channelObject);
        array.append(item);
    }

    QByteArray raw = QJsonDocument(array).toJson(QJsonDocument::Compact);
    FakeNetworkReply *reply = new FakeNetworkReply(raw, m_dispatchercontrol);

    QObject::connect(reply, SIGNAL(readyRead()), m_dispatchercontrol, SLOT(finishVerification()));
    reply->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.count(), 3);
    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_OK").count(), 2);
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_OK").contains(11));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_OK").contains(12));
    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:hash").count(), 1);
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:hash").contains(13));

    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.count(), 1);
    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_WAIT").count(), 1);
    QVERIFY(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_WAIT").contains(14));
}
