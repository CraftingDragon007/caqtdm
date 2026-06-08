#include "tst_caqtdm_lib.h"

void TestCaQtDM_Lib::initTestCase()
{
    // code to be executed before the first test function

    m_fakeFileOpenWindow = Q_NULLPTR;
    m_mutexKnobData = Q_NULLPTR;
    m_caQtDM_Lib = Q_NULLPTR;
}

void TestCaQtDM_Lib::init()
{
    // code to be executed before each test function

    m_fakeFileOpenWindow = new FakeFileOpenWindow(Q_NULLPTR);
    caLed *parentAS = new caLed(m_fakeFileOpenWindow);
    m_mutexKnobData = new MutexKnobData();
    m_caQtDM_Lib = new CaQtDM_Lib(m_fakeFileOpenWindow,
                                  "",
                                  "",
                                  m_mutexKnobData,
                                  {},
                                  Q_NULLPTR,
                                  false,
                                  parentAS,
                                  {});
}

void TestCaQtDM_Lib::cleanupTestCase()
{
    // code to be executed after the last test function
}

void TestCaQtDM_Lib::cleanup()
{
    // code to be executed after each test function

    delete m_caQtDM_Lib;
    delete m_mutexKnobData;
    delete m_fakeFileOpenWindow;
}

void TestCaQtDM_Lib::checkJsonStringWorks()
{
    QString emptyJson = "{}";
    QCOMPARE(m_caQtDM_Lib->checkJsonString(emptyJson), true);

    QString validJson
        = R"({"root":{"children":[{"id":"node-1","metadata":{"tags":["alpha","beta"],"priority":7}},{"id":"node-2","metadata":{"tags":[],"priority":0}}],"configuration":{"featureFlags":{"enableQuantumMode":false,"useLegacyFluxCapacitor":true},"thresholds":[0.1,0.5,0.9]},"auditTrail":[{"timestamp":"1970-01-01T00:00:00Z","action":"INITIALIZE"},{"timestamp":"2099-12-31T23:59:59Z","action":"TERMINATE"}],"nullableField":null,"emptyObject":{},"emptyArray":[]}})";
    QCOMPARE(m_caQtDM_Lib->checkJsonString(validJson), true);

    QString validMultilineJson = R"({
        "a": 1,
        "caqtdm_monitor": {
            "maxdisplayrate": 42
        },
        "b": 2
    })";
    QCOMPARE(m_caQtDM_Lib->checkJsonString(validMultilineJson), true);

    QString emptyString = "";
    QCOMPARE(m_caQtDM_Lib->checkJsonString(emptyString), false);

    QString invalidJson
        = R"({"root":{"children":[{"id":"node-1","metadata":{"tags":["alpha","beta"],"priority":7}},{"id":"node-2","metadata":{"tags":[],"priority":0}}],"configuration":{"featureFlags":{"enableQuantumMode":false,"useLegacyFluxCapacitor":true},"thresholds":[0.1,0.5,0.9]},"auditTrail":[{"timestamp":"1970-01-01T00:00:00Z","action"2099-12-31T23:59:59Z","action":"TERMINATE"}],"nullableField":null,"emptyObject":{},"emptyArray":[]}})";
    QCOMPARE(m_caQtDM_Lib->checkJsonString(invalidJson), false);

    QString invalidJsonMissingBracket
        = R"({"root":{"children":[{"id":"node-1","metadata":{"tags":["alpha","beta"],"priority":7}},{"id":"node-2","metadata":{"tags":[],"priority":0}}],"configuration":{"featureFlags":{"enableQuantumMode":false,"useLegacyFluxCapacitor":true},"thresholds":[0.1,0.5,0.9]},"auditTrail":[{"timestamp":"1970-01-01T00:00:00Z","action":"INITIALIZE"},{"timestamp":"2099-12-31T23:59:59Z","action":"TERMINATE"}],"nullableField":null,"emptyObject":{},"emptyArray":[]})";
    QCOMPARE(m_caQtDM_Lib->checkJsonString(invalidJsonMissingBracket), false);
}

void TestCaQtDM_Lib::parseForDisplayRateWorks()
{
    QString json = R"({"a": 1,"b": 2})";
    int rate = -1;

    int result = m_caQtDM_Lib->parseForDisplayRate(json, rate);
    QCOMPARE(result, 0);
    QCOMPARE(rate, -1);

    json = R"({
        "a": 1,
        "caqtdm_monitor": {
            "maxdisplayrate": 42
        },
        "b": 2
    })";
    rate = 0;

    result = m_caQtDM_Lib->parseForDisplayRate(json, rate);
    QCOMPARE(result, 1);
    QCOMPARE(rate, 42);

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(doc.isObject(), true);

    QJsonObject root = doc.object();
    QCOMPARE(root.contains("caqtdm_monitor"), false);
    QCOMPARE(root["a"].toInt(), 1);
    QCOMPARE(root["b"].toInt(), 2);

    json = R"({"arr":{"e":-1,"i":1,"s":1},"caqtdm_monitor":{"maxdisplayrate":1},"dbnd":{"abs":0},"sync":{"before":""}})";
    rate = 0;

    result = m_caQtDM_Lib->parseForDisplayRate(json, rate);
    qDebug() << json;
    QCOMPARE(result, 1);
    QCOMPARE(rate, 1);

    doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QCOMPARE(doc.isObject(), true);

    root = doc.object();
    QCOMPARE(root.contains("caqtdm_monitor"), false);
    QCOMPARE(root["arr"].toObject(), QJsonObject({{"e", -1}, {"i", 1}, {"s", 1}}));
    QCOMPARE(root["dbnd"].toObject(), QJsonObject({{"abs", 0}}));
    QCOMPARE(root["sync"].toObject(), QJsonObject({{"before", ""}}));
}

void TestCaQtDM_Lib::parseForQRectConstWorks()
{
    QString json = R"({"badkey":[1,2,3]})";
    double valueArray[3] = {0.0, 0.0, 0.0};

    QCOMPARE(m_caQtDM_Lib->parseForQRectConst(json, valueArray), false);
    QCOMPARE(valueArray[0], 0.0);
    QCOMPARE(valueArray[1], 0.0);
    QCOMPARE(valueArray[2], 0.0);
    QCOMPARE(json, QString(R"({"badkey":[1,2,3]})"));

    json = R"({"valueconst":[1,2,3]})";
    valueArray[0] = 1.0;
    valueArray[1] = 2.0;
    valueArray[2] = 3.0;

    QCOMPARE(m_caQtDM_Lib->parseForQRectConst(json, valueArray), true);
    QCOMPARE(valueArray[0], 1.0);
    QCOMPARE(valueArray[1], 2.0);
    QCOMPARE(valueArray[2], 3.0);
    QCOMPARE(json, QString(R"({"valueconst":[1,2,3]})"));
}

void TestCaQtDM_Lib::treatMacroWorks()
{
    m_caQtDM_Lib->unknownMacrosList.clear();
    m_caQtDM_Lib->level = 0;
    m_caQtDM_Lib->savedFile[0] = "test.ui";

    {
        QMap<QString, QString> map;
        map["A"] = "$(B)";
        map["B"] = "$(C)";
        map["C"] = "final";

        QString text = "prefix $(A) suffix";
        bool doNothing = true;

        QCOMPARE(m_caQtDM_Lib->treatMacro(map, text, &doNothing, "widget"),
                 QString("prefix final suffix"));
        QCOMPARE(doNothing, false);
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.isEmpty(), true);
    }

    m_caQtDM_Lib->unknownMacrosList.clear();

    {
        QMap<QString, QString> map;
        map["A"] = "literal";

        QString text = R"(pv $(A{"regex":"literal","value":"JSON"}))";
        bool doNothing = true;

        QCOMPARE(m_caQtDM_Lib->treatMacro(map, text, &doNothing, "widget"), QString("pv JSON"));
        QCOMPARE(doNothing, false);
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.isEmpty(), true);
    }

    m_caQtDM_Lib->unknownMacrosList.clear();

    {
        QMap<QString, QString> map;
        map["A"] = "prefix-123-suffix";

        QString text = R"(pv $(A{"regex":"[0-9]+","value":"456"}))";
        bool doNothing = true;

        QCOMPARE(m_caQtDM_Lib->treatMacro(map, text, &doNothing, "widget"),
                 QString("pv prefix-456-suffix"));
        QCOMPARE(doNothing, false);
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.isEmpty(), true);
    }

    m_caQtDM_Lib->unknownMacrosList.clear();

    {
        QMap<QString, QString> map;
        map["A"] = "prefix-123-suffix";
        map["B"] = "456";

        QString text = R"lim(pv $(A{"regex":"[0-9]+","value":"$(B)"}))lim";
        bool doNothing = true;

        QCOMPARE(m_caQtDM_Lib->treatMacro(map, text, &doNothing, "widget"),
                 QString("pv prefix-456-suffix"));
        QCOMPARE(doNothing, false);
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.isEmpty(), true);
    }

    m_caQtDM_Lib->unknownMacrosList.clear();

    {
        QMap<QString, QString> map;
        map["A"] = "one";

        QString text = "pv $(MISSING)";
        bool doNothing = false;

        QCOMPARE(m_caQtDM_Lib->treatMacro(map, text, &doNothing, "widget42"),
                 QString("pv $(MISSING)"));
        QCOMPARE(doNothing, false);
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.size(), 1);

        QString key = "(MISSING)###widget42###test.ui";
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.contains(key), true);
        QCOMPARE(m_caQtDM_Lib->unknownMacrosList.value(key), QString("test.ui"));
    }

    {
        QMap<QString, QString> map;
        QString text = "pv $(A)";
        bool doNothing = false;

        QCOMPARE(m_caQtDM_Lib->treatMacro(map, text, &doNothing, "widget"), QString("pv $(A)"));
        QCOMPARE(doNothing, true);
    }
}
