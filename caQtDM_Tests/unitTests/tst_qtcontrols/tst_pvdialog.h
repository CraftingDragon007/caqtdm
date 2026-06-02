#ifndef TST_PVDIALOG_H
#define TST_PVDIALOG_H

#include "fakeformwindow.h"
#include <pvdialog.h>

#include <QObject>
#include <QTest>

class TestPVDialog : public QObject
{
    Q_OBJECT
public:
    TestPVDialog() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void savesPlainChannelIfNothingIsSet();
    void savesPrefix();
    void savesDeadband();
    void savesMaxDisplayRate();
    void savesDecimation();
    void savesArray();
    void savesSync();
    void savesTs();
    void savesEverythingAtOnce();
    void savesNothingWithoutPV();

private:
    void parseChannel(const QString &channel, QString &outPv, QJsonObject &outJson);
    QString getChannelData();

    PVDialog *m_dialog;
    FakeFormWindow *m_formWindow;
};

#endif // TST_PVDIALOG_H
