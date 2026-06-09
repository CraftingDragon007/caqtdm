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
    bsread_Decode *firstConnection = m_dispatchercontrol->bsreadconnections.first();
    QThread *firstThread = m_dispatchercontrol->bsreadThreads.first();
    QCOMPARE(QString(firstConnection->getConnectionPoint()), QString("tcp://127.0.0.1:6000"));
    QVERIFY(m_dispatchercontrol->tobeRemoved.isEmpty());

    QByteArray rawSecond
        = R"({"configuration":{"streamType":"push_pull"},"stream":"tcp://127.0.0.1:6001"})";
    FakeNetworkReply *replySecond = new FakeNetworkReply(rawSecond, m_dispatchercontrol);

    QObject::connect(replySecond,
                     SIGNAL(readyRead()),
                     m_dispatchercontrol,
                     SLOT(finishReplyConnect()));
    replySecond->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->streams.count(), 1);
    QCOMPARE(m_dispatchercontrol->streams.first(), QString("tcp://127.0.0.1:6001"));

    QCOMPARE(m_dispatchercontrol->bsreadconnections.count(), 1);
    QCOMPARE(m_dispatchercontrol->bsreadThreads.count(), 1);
    QVERIFY(m_dispatchercontrol->bsreadconnections.first());
    QVERIFY(m_dispatchercontrol->bsreadconnections.first() != firstConnection);
    QVERIFY(m_dispatchercontrol->bsreadThreads.first() != firstThread);
    QCOMPARE(QString(m_dispatchercontrol->bsreadconnections.first()->getConnectionPoint()),
             QString("tcp://127.0.0.1:6001"));

    m_dispatchercontrol->tobeRemoved.clear();

    QByteArray rawException
        = R"({"exception":"java.lang.IllegalArgumentException: bad request","message":"recorded: CH_A - 1,CH_B - 2,CH_C - 3"})";
    FakeNetworkReply *replyException = new FakeNetworkReply(rawException, m_dispatchercontrol);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*ErrorChannels\\.count\\(\\): 3$"));

    QObject::connect(replyException,
                     SIGNAL(readyRead()),
                     m_dispatchercontrol,
                     SLOT(finishReplyConnect()));
    replyException->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->bsreadconnections.count(), 1);
    QCOMPARE(m_dispatchercontrol->bsreadThreads.count(), 1);
    QVERIFY(m_dispatchercontrol->tobeRemoved.contains("CH_A"));
    QVERIFY(m_dispatchercontrol->tobeRemoved.contains("CH_B"));
    QCOMPARE(m_dispatchercontrol->tobeRemoved.count(), 2);
}

void Testbsread_dispatchercontrol::finishVerificationWorks()
{
    m_dispatchercontrol->ChannelsApprovePipeline.clear();
    m_dispatchercontrol->ChannelsToBeApprovePipeline.clear();

    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_OK", 11);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_OK", 12);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_WAIT", 14);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_UNUSED", 15);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:hash", 13);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:pulse_id", 16);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:htype", 17);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:global_timestamp_ns", 18);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:global_timestamp_sec", 19);

    QByteArray raw = R"([
        {
            "recording": true,
            "channel": {
                "name": "CH_OK"
            }
        },
        {
            "recording": false,
            "channel": {
                "name": "CH_WAIT"
            }
        },
        {
            "recording": true,
            "channel": {
                "name": "CH_UNKNOWN"
            }
        }
    ])";
    FakeNetworkReply *reply = new FakeNetworkReply(raw, m_dispatchercontrol);

    QObject::connect(reply, SIGNAL(readyRead()), m_dispatchercontrol, SLOT(finishVerification()));
    reply->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.count(), 7);
    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_OK").count(), 2);
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_OK").contains(11));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_OK").contains(12));

    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:hash").contains(13));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:pulse_id").contains(16));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:htype").contains(17));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:global_timestamp_ns")
                .contains(18));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:global_timestamp_sec")
                .contains(19));

    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.count(), 2);
    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_WAIT").count(), 1);
    QVERIFY(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_WAIT").contains(14));
    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_UNUSED").count(), 1);
    QVERIFY(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_UNUSED").contains(15));

    m_dispatchercontrol->ChannelsApprovePipeline.clear();
    m_dispatchercontrol->ChannelsToBeApprovePipeline.clear();

    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("bsread:pulse_id", 21);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_PENDING", 22);

    QByteArray rawObject = R"({"notArray":true})";
    FakeNetworkReply *replyObject = new FakeNetworkReply(rawObject, m_dispatchercontrol);

    QObject::connect(replyObject,
                     SIGNAL(readyRead()),
                     m_dispatchercontrol,
                     SLOT(finishVerification()));
    replyObject->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.count(), 1);
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("bsread:pulse_id").contains(21));
    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.count(), 1);
    QVERIFY(m_dispatchercontrol->ChannelsToBeApprovePipeline.values("CH_PENDING").contains(22));

    m_dispatchercontrol->ChannelsApprovePipeline.clear();
    m_dispatchercontrol->ChannelsToBeApprovePipeline.clear();

    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_FALLBACK", 31);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_FALLBACK", 32);
    m_dispatchercontrol->ChannelsToBeApprovePipeline.insert("CH_EXTRA", 33);

    QByteArray rawException = R"({"exception":"boom"})";
    FakeNetworkReply *replyException = new FakeNetworkReply(rawException, m_dispatchercontrol);

    QObject::connect(replyException,
                     SIGNAL(readyRead()),
                     m_dispatchercontrol,
                     SLOT(finishVerification()));
    replyException->triggerReadyRead();

    QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.count(), 3);
    QCOMPARE(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_FALLBACK").count(), 2);
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_FALLBACK").contains(31));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_FALLBACK").contains(32));
    QVERIFY(m_dispatchercontrol->ChannelsApprovePipeline.values("CH_EXTRA").contains(33));
    QCOMPARE(m_dispatchercontrol->ChannelsToBeApprovePipeline.count(), 0);
}
