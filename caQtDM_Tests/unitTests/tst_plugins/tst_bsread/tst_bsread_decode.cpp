#include "tst_bsread_decode.h"
#include "zmq.h"

void Testbsread_Decode::initTestCase()
{
    // code to be executed before the first test function

    m_zmqContext = Q_NULLPTR;
    m_decode = Q_NULLPTR;
}

void Testbsread_Decode::init()
{
    // code to be executed before each test function

    m_zmqContext = zmq_ctx_new();
    m_decode = new bsread_Decode(m_zmqContext, "tcp://127.0.0.1:5555");
}

void Testbsread_Decode::cleanupTestCase()
{
    // code to be executed after the last test function
}

void Testbsread_Decode::cleanup()
{
    // code to be executed after each test function

    delete m_decode;
    zmq_ctx_term(m_zmqContext);
}

void Testbsread_Decode::parsesMainHeader()
{
    QByteArray raw
        = R"({"hash":"deadbeef","pulse_id":42,"htype":"bsr_m-1.1","global_timestamp":{"epoch":1000.5,"ns":999.0,"sec":12345.0,"ns_offset":7.0}})";
    bool ok = m_decode->setMainHeader(raw.data(), raw.size());

    QVERIFY(ok);
    QCOMPARE(m_decode->channelcounter, 0);
    QCOMPARE(m_decode->hash, QString("deadbeef"));
    QCOMPARE(m_decode->pulse_id, 42.0);
    QCOMPARE(m_decode->main_htype, QString("bsr_m-1.1"));
    QCOMPARE(m_decode->global_timestamp_epoch, 1000);
    QCOMPARE(m_decode->global_timestamp_ns, 999);
    QCOMPARE(m_decode->global_timestamp_sec, 12345);
    QCOMPARE(m_decode->global_timestamp_ns_offset, 7);

    QByteArray rawExtremes
        = R"({"hash":"deadbeef","pulse_id":12432145123.2,"htype":"bsr_m-1.1","global_timestamp":{"epoch":2147483647,"ns":2147483647,"sec":2147483647,"ns_offset":-2147483648}})";
    ok = m_decode->setMainHeader(rawExtremes.data(), rawExtremes.size());

    QVERIFY(ok);
    QCOMPARE(m_decode->channelcounter, 0);
    QCOMPARE(m_decode->hash, QString("deadbeef"));
    QCOMPARE(m_decode->pulse_id, 12432145123.2);
    QCOMPARE(m_decode->main_htype, QString("bsr_m-1.1"));
    QCOMPARE(m_decode->global_timestamp_epoch, INT32_MAX);
    QCOMPARE(m_decode->global_timestamp_ns, INT32_MAX);
    QCOMPARE(m_decode->global_timestamp_sec, INT32_MAX);
    QCOMPARE(m_decode->global_timestamp_ns_offset, INT32_MIN);
}

void Testbsread_Decode::parsesHeader()
{
    QByteArray raw = R"({"channels":[
        {"name":"CH_F64","type":"float64","offset":2,"modulo":5},
        {"name":"CH_F32","type":"float32"},
        {"name":"CH_I64","type":"int64"},
        {"name":"CH_I32","type":"int32"},
        {"name":"CH_U64","type":"uint64"},
        {"name":"CH_U32","type":"uint32"},
        {"name":"CH_I16","type":"int16"},
        {"name":"CH_U16","type":"uint16"},
        {"name":"CH_I8","type":"int8"},
        {"name":"CH_U8","type":"uint8"},
        {"name":"CH_BOOL","type":"bool"},
        {"name":"CH_STR","type":"string"},
        {"name":"CH_NONE","type":"unknown"},
        {"name":"CH_BIG","type":"float32","encoding":"big"},
        {"name":"CH_ENC","type":"float32","encoding":"little,lz4,deflate"},
        {"name":"CH_SHAPE","type":"float32","shape":[3,4]}
    ],"pulse_id":100.0,"htype":"bsr_d-1.1","global_timestamp":{"sec":2147483647,"ns":123456}})";

    m_decode->setHeader(raw.data(), raw.size());

    QCOMPARE(m_decode->ChannelSearch["CH_F64"]->type, bs_float64);
    QCOMPARE(m_decode->ChannelSearch["CH_F32"]->type, bs_float32);
    QCOMPARE(m_decode->ChannelSearch["CH_I64"]->type, bs_int64);
    QCOMPARE(m_decode->ChannelSearch["CH_I32"]->type, bs_int32);
    QCOMPARE(m_decode->ChannelSearch["CH_U64"]->type, bs_uint64);
    QCOMPARE(m_decode->ChannelSearch["CH_U32"]->type, bs_uint32);
    QCOMPARE(m_decode->ChannelSearch["CH_I16"]->type, bs_int16);
    QCOMPARE(m_decode->ChannelSearch["CH_U16"]->type, bs_uint16);
    QCOMPARE(m_decode->ChannelSearch["CH_I8"]->type, bs_int8);
    QCOMPARE(m_decode->ChannelSearch["CH_U8"]->type, bs_uint8);
    QCOMPARE(m_decode->ChannelSearch["CH_BOOL"]->type, bs_bool);
    QCOMPARE(m_decode->ChannelSearch["CH_STR"]->type, bs_string);
    QCOMPARE(m_decode->ChannelSearch["CH_NONE"]->type, bs_none);

    QCOMPARE(m_decode->ChannelSearch["CH_F64"]->offset, 2);
    QCOMPARE(m_decode->ChannelSearch["CH_F64"]->modulo, 5);

    QCOMPARE(m_decode->ChannelSearch["CH_BIG"]->endianess, bs_big);
    QVERIFY2(m_decode->ChannelSearch.contains("CH_BIG.ENC_GROUP"),
             "Expected ENC_GROUP sub-channel for 'big'");
    QCOMPARE(m_decode->ChannelSearch["CH_BIG.ENC_GROUP"]->bsdata.bs_string, QString("big"));

    QVERIFY2(m_decode->ChannelSearch.contains("CH_ENC.ENC_GROUP"), "Expected ENC_GROUP sub-channel");
    QVERIFY2(m_decode->ChannelSearch.contains("CH_ENC.ENC_TYPE"), "Expected ENC_TYPE sub-channel");
    QVERIFY2(m_decode->ChannelSearch.contains("CH_ENC.ENC_SUBTYPE"),
             "Expected ENC_SUBTYPE sub-channel");
    QCOMPARE(m_decode->ChannelSearch["CH_ENC.ENC_GROUP"]->bsdata.bs_string, QString("little"));
    QCOMPARE(m_decode->ChannelSearch["CH_ENC.ENC_TYPE"]->bsdata.bs_string, QString("lz4"));
    QCOMPARE(m_decode->ChannelSearch["CH_ENC.ENC_SUBTYPE"]->bsdata.bs_string, QString("deflate"));

    QVERIFY2(m_decode->ChannelSearch.contains("CH_SHAPE.BSREADSHAPE0"),
             "Expected BSREADSHAPE0 sub-channel");
    QVERIFY2(m_decode->ChannelSearch.contains("CH_SHAPE.BSREADSHAPE1"),
             "Expected BSREADSHAPE1 sub-channel");
    QCOMPARE(m_decode->ChannelSearch["CH_SHAPE.BSREADSHAPE0"]->bsdata.bs_float32, 3.0f);
    QCOMPARE(m_decode->ChannelSearch["CH_SHAPE.BSREADSHAPE1"]->bsdata.bs_float32, 4.0f);
    QCOMPARE(m_decode->ChannelSearch["CH_SHAPE"]->shape.size(), 2);
    QCOMPARE(m_decode->ChannelSearch["CH_SHAPE"]->shape[0], 3);
    QCOMPARE(m_decode->ChannelSearch["CH_SHAPE"]->shape[1], 4);

    QCOMPARE(m_decode->pulse_id, 100.0);
    QCOMPARE(m_decode->main_htype, QString("bsr_d-1.1"));
    QCOMPARE(m_decode->global_timestamp_sec, 2147483647);
    QCOMPARE(m_decode->global_timestamp_ns, 123456);
}
