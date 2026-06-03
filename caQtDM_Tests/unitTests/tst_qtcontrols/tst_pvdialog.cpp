#include "tst_pvdialog.h"

void TestPVDialog::parseChannel(const QString &channel, QString &outPv, QJsonObject &outJson)
{
    outJson = {};
    int dotPos = channel.indexOf(".{"); // first occurrence of ".{"
    if (dotPos == -1) {
        outPv = channel;
    } else {
        outPv = channel.left(dotPos);
        QByteArray jsonPart = channel.mid(dotPos + 1).toUtf8();
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonPart, &err);
        QVERIFY2(err.error == QJsonParseError::NoError,
                 qPrintable("JSON parse error: " + err.errorString()));
        outJson = doc.object();
    }
}

QString TestPVDialog::getChannelData()
{
    // caCamera branch writes "channelData", everything else "channel"
    if (m_formWindow->fakeCursor()->properties.contains("channelData"))
        return m_formWindow->fakeCursor()->properties.value("channelData").toString();
    return m_formWindow->fakeCursor()->properties.value("channel").toString();
}

void TestPVDialog::initTestCase()
{
    // code to be executed before the first test function

    m_formWindow = Q_NULLPTR;
    m_dialog = Q_NULLPTR;
}

void TestPVDialog::init()
{
    // code to be executed before each test function

    m_formWindow = new FakeFormWindow();
    caLed *entry = new caLed(m_formWindow);
    m_dialog = new PVDialog(entry, Q_NULLPTR);
}

void TestPVDialog::cleanupTestCase()
{
    // code to be executed after the last test function
}

void TestPVDialog::cleanup()
{
    // code to be executed after each test function

    delete m_dialog;
    delete m_formWindow;
}

void TestPVDialog::savesPlainChannelIfNothingIsSet()
{
    m_dialog->pvLine->setPlainText("TEST:PV:1");
    m_dialog->prefixComboBox->setCurrentText("");

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QCOMPARE(pv, QString("TEST:PV:1"));
    QVERIFY2(json.isEmpty(), "Expected no JSON when all flags off");
}

void TestPVDialog::savesPrefix()
{
    m_dialog->pvLine->setPlainText("TEST:PV:2");
    m_dialog->prefixComboBox->setCurrentText("epics3");

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QVERIFY2(channel.startsWith("epics3://TEST:PV:2"), qPrintable("Unexpected channel: " + channel));
}

void TestPVDialog::savesDeadband()
{
    m_dialog->pvLine->setPlainText("TEST:PV:DBND");
    m_dialog->dbndCheckBox->setChecked(true);
    m_dialog->dbndComboBox->setCurrentText("abs");
    m_dialog->dbndDoubleValue->setValue(0.5);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QVERIFY2(json.contains("dbnd"), "Expected 'dbnd' key");
    QJsonObject dbndObj = json["dbnd"].toObject();
    QVERIFY2(dbndObj.contains("abs"), "Expected 'abs' key inside dbnd");
    QCOMPARE(dbndObj["abs"].toDouble(), 0.5);
}

void TestPVDialog::savesMaxDisplayRate()
{
    m_dialog->pvLine->setPlainText("TEST:PV:RATE");
    m_dialog->rateCheckBox->setChecked(true);
    m_dialog->rateIntValue->setValue(25);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QVERIFY2(json.contains("caqtdm_monitor"), "Expected 'caqtdm_monitor' key");
    QJsonObject mon = json["caqtdm_monitor"].toObject();
    QCOMPARE(mon["maxdisplayrate"].toInt(), 25);
}

void TestPVDialog::savesDecimation()
{
    m_dialog->pvLine->setPlainText("TEST:PV:DEC");
    m_dialog->decCheckBox->setChecked(true);
    m_dialog->decIntValue->setValue(3);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QVERIFY2(json.contains("dec"), "Expected 'dec' key");
    QCOMPARE(json["dec"].toObject()["n"].toInt(), 3);
}

void TestPVDialog::savesArray()
{
    m_dialog->pvLine->setPlainText("TEST:PV:ARR");
    m_dialog->arrayCheckBox->setChecked(true);
    m_dialog->arrayIntValue_s->setValue(0);
    m_dialog->arrayIntValue_i->setValue(1);
    m_dialog->arrayIntValue_e->setValue(9);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QVERIFY2(json.contains("arr"), "Expected 'arr' key");
    QJsonObject arr = json["arr"].toObject();
    QCOMPARE(arr["s"].toInt(), 0);
    QCOMPARE(arr["i"].toInt(), 1);
    QCOMPARE(arr["e"].toInt(), 9);
}

void TestPVDialog::savesSync()
{
    m_dialog->pvLine->setPlainText("TEST:PV:SYNC");
    m_dialog->syncCheckBox->setChecked(true);
    m_dialog->syncComboBox->setCurrentText("before");
    m_dialog->syncLine->setText("SYNC:TRIGGER:PV");

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QVERIFY2(json.contains("sync"), "Expected 'sync' key");
    QJsonObject syncObj = json["sync"].toObject();
    QVERIFY2(syncObj.contains("before"), "Expected 'before' key inside sync");
    QCOMPARE(syncObj["before"].toString(), QString("SYNC:TRIGGER:PV"));
}

void TestPVDialog::savesTs()
{
    m_dialog->pvLine->setPlainText("TEST:PV:TS");
    m_dialog->tsCheckBox->setChecked(true);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QVERIFY2(json.contains("ts"), "Expected 'ts' key");
    QVERIFY2(json["ts"].toObject().isEmpty(), "'ts' value should be an empty JSON object");
}

void TestPVDialog::savesEverythingAtOnce()
{
    m_dialog->pvLine->setPlainText("TEST:PV:ALL");
    m_dialog->dbndCheckBox->setChecked(true);
    m_dialog->rateCheckBox->setChecked(true);
    m_dialog->decCheckBox->setChecked(true);
    m_dialog->arrayCheckBox->setChecked(true);
    m_dialog->syncCheckBox->setChecked(true);
    m_dialog->tsCheckBox->setChecked(true);

    m_dialog->dbndDoubleValue->setValue(1.23);
    m_dialog->rateIntValue->setValue(50);
    m_dialog->decIntValue->setValue(4);
    m_dialog->arrayIntValue_s->setValue(2);
    m_dialog->arrayIntValue_i->setValue(3);
    m_dialog->arrayIntValue_e->setValue(99);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QString pv;
    QJsonObject json;
    parseChannel(channel, pv, json);

    QCOMPARE(pv, QString("TEST:PV:ALL"));

    QVERIFY(json.contains("dbnd"));
    QJsonObject dbndObj = json["dbnd"].toObject();
    QVERIFY2(dbndObj.contains("abs"), "Expected 'abs' key inside dbnd");
    QCOMPARE(dbndObj["abs"].toDouble(), 1.23);

    QVERIFY(json.contains("caqtdm_monitor"));
    QCOMPARE(json["caqtdm_monitor"].toObject()["maxdisplayrate"].toInt(), 50);

    QVERIFY(json.contains("dec"));
    QCOMPARE(json["dec"].toObject()["n"].toInt(), 4);

    QVERIFY(json.contains("arr"));
    QJsonObject arrObj = json["arr"].toObject();
    QCOMPARE(arrObj["s"].toInt(), 2);
    QCOMPARE(arrObj["i"].toInt(), 3);
    QCOMPARE(arrObj["e"].toInt(), 99);

    QVERIFY(json.contains("sync"));
    QJsonObject syncObj = json["sync"].toObject();
    QCOMPARE(syncObj.keys().size(), 1);

    QVERIFY(json.contains("ts"));
    QVERIFY2(json["ts"].toObject().isEmpty(), "'ts' value should be an empty JSON object");
}

void TestPVDialog::savesNothingWithoutPV()
{
    m_dialog->pvLine->setPlainText("");
    m_dialog->rateCheckBox->setChecked(true);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();
    QVERIFY2(channel.isEmpty(), "Channel must be empty when PV text is empty");

    m_dialog->rateCheckBox->setChecked(false);
}

void TestPVDialog::constructorParsesPlainChannel()
{
    caLed entry(Q_NULLPTR);
    entry.setPV("TEST:PV:PLAIN");
    PVDialog dlg(&entry, Q_NULLPTR);

    QCOMPARE(dlg.pvLine->toPlainText(), QString("TEST:PV:PLAIN"));
    QVERIFY2(!dlg.dbndCheckBox->isChecked(), "deadband should be off");
    QVERIFY2(!dlg.rateCheckBox->isChecked(), "rate should be off");
    QVERIFY2(!dlg.decCheckBox->isChecked(), "decimation should be off");
    QVERIFY2(!dlg.arrayCheckBox->isChecked(), "array should be off");
    QVERIFY2(!dlg.syncCheckBox->isChecked(), "sync should be off");
    QVERIFY2(!dlg.tsCheckBox->isChecked(), "ts should be off");
}

void TestPVDialog::constructorParsesSomeFilters()
{
    caLed entry(Q_NULLPTR);
    entry.setPV(R"(TEST:PV:SOME.{"dbnd":{"abs":1.5},"dec":{"n":7}})");
    PVDialog dlg(&entry, Q_NULLPTR);

    QCOMPARE(dlg.pvLine->toPlainText(), QString("TEST:PV:SOME"));

    QVERIFY2(dlg.dbndCheckBox->isChecked(), "deadband should be on");
    QCOMPARE(dlg.dbndComboBox->currentText(), QString("abs"));
    QCOMPARE(dlg.dbndDoubleValue->value(), 1.5);

    QVERIFY2(dlg.decCheckBox->isChecked(), "decimation should be on");
    QCOMPARE(dlg.decIntValue->value(), 7);

    QVERIFY2(!dlg.rateCheckBox->isChecked(), "rate should be off");
    QVERIFY2(!dlg.arrayCheckBox->isChecked(), "array should be off");
    QVERIFY2(!dlg.syncCheckBox->isChecked(), "sync should be off");
    QVERIFY2(!dlg.tsCheckBox->isChecked(), "ts should be off");
}

void TestPVDialog::constructorParsesAllFilters()
{
    caLed entry(Q_NULLPTR);
    entry.setPV(
        R"(epics3://TEST:PV:ALL.{"dbnd":{"rel":0.25},"caqtdm_monitor":{"maxdisplayrate":30},"dec":{"n":5},"arr":{"s":2,"i":3,"e":99},"sync":{"while":"SYNC:TRIGGER:PV"},"ts":{}})");
    PVDialog dlg(&entry, Q_NULLPTR);

    QCOMPARE(dlg.pvLine->toPlainText(), QString("TEST:PV:ALL"));
    QCOMPARE(dlg.prefixComboBox->currentText(), QString("epics3"));

    QVERIFY2(dlg.dbndCheckBox->isChecked(), "deadband should be on");
    QCOMPARE(dlg.dbndComboBox->currentText(), QString("rel"));
    QCOMPARE(dlg.dbndDoubleValue->value(), 0.25);

    QVERIFY2(dlg.rateCheckBox->isChecked(), "rate should be on");
    QCOMPARE(dlg.rateIntValue->value(), 30);

    QVERIFY2(dlg.decCheckBox->isChecked(), "decimation should be on");
    QCOMPARE(dlg.decIntValue->value(), 5);

    QVERIFY2(dlg.arrayCheckBox->isChecked(), "array should be on");
    QCOMPARE(dlg.arrayIntValue_s->value(), 2);
    QCOMPARE(dlg.arrayIntValue_i->value(), 3);
    QCOMPARE(dlg.arrayIntValue_e->value(), 99);

    QVERIFY2(dlg.syncCheckBox->isChecked(), "sync should be on");
    QCOMPARE(dlg.syncComboBox->currentText(), QString("while"));
    QCOMPARE(dlg.syncLine->text(), QString("SYNC:TRIGGER:PV"));

    QVERIFY2(dlg.tsCheckBox->isChecked(), "ts should be on");
    QVERIFY2(dlg.msgLine->text().isEmpty(), "no parse errors expected");
}

void TestPVDialog::roundTripSaveThenParse()
{
    // save phase: configure m_dialog and call saveState
    m_dialog->pvLine->setPlainText("TEST:PV:ROUNDTRIP");
    m_dialog->prefixComboBox->setCurrentText("epics4");

    m_dialog->dbndCheckBox->setChecked(true);
    m_dialog->dbndComboBox->setCurrentText("abs");
    m_dialog->dbndDoubleValue->setValue(0.75);

    m_dialog->rateCheckBox->setChecked(true);
    m_dialog->rateIntValue->setValue(20);

    m_dialog->decCheckBox->setChecked(true);
    m_dialog->decIntValue->setValue(6);

    m_dialog->arrayCheckBox->setChecked(true);
    m_dialog->arrayIntValue_s->setValue(1);
    m_dialog->arrayIntValue_i->setValue(2);
    m_dialog->arrayIntValue_e->setValue(50);

    m_dialog->syncCheckBox->setChecked(true);
    m_dialog->syncComboBox->setCurrentText("last");
    m_dialog->syncLine->setText("ROUND:TRIP:PV");

    m_dialog->tsCheckBox->setChecked(true);

    m_formWindow->fakeCursor()->properties.clear();
    m_dialog->saveState();
    QString channel = getChannelData();

    // parse phase: feed the saved channel back into a new PVDialog
    caLed entry(Q_NULLPTR);
    entry.setPV(channel);
    PVDialog dlg(&entry, Q_NULLPTR);

    QCOMPARE(dlg.pvLine->toPlainText(), QString("TEST:PV:ROUNDTRIP"));
    QCOMPARE(dlg.prefixComboBox->currentText(), QString("epics4"));

    QVERIFY(dlg.dbndCheckBox->isChecked());
    QCOMPARE(dlg.dbndComboBox->currentText(), QString("abs"));
    QCOMPARE(dlg.dbndDoubleValue->value(), 0.75);

    QVERIFY(dlg.rateCheckBox->isChecked());
    QCOMPARE(dlg.rateIntValue->value(), 20);

    QVERIFY(dlg.decCheckBox->isChecked());
    QCOMPARE(dlg.decIntValue->value(), 6);

    QVERIFY(dlg.arrayCheckBox->isChecked());
    QCOMPARE(dlg.arrayIntValue_s->value(), 1);
    QCOMPARE(dlg.arrayIntValue_i->value(), 2);
    QCOMPARE(dlg.arrayIntValue_e->value(), 50);

    QVERIFY(dlg.syncCheckBox->isChecked());
    QCOMPARE(dlg.syncComboBox->currentText(), QString("last"));
    QCOMPARE(dlg.syncLine->text(), QString("ROUND:TRIP:PV"));

    QVERIFY(dlg.tsCheckBox->isChecked());
    QVERIFY2(dlg.msgLine->text().isEmpty(), "no parse errors expected after round-trip");
}
