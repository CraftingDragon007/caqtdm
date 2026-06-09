#ifndef TST_BSREAD_DISPATCHERCONTROL_H
#define TST_BSREAD_DISPATCHERCONTROL_H

#include <bsread_dispatchercontrol.h>

#include <QObject>
#include <QTest>

class Testbsread_dispatchercontrol : public QObject
{
    Q_OBJECT
public:
    Testbsread_dispatchercontrol() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void finishReplyConnectWorks();
    void finishVerificationWorks();

private:
    bsread_dispatchercontrol *m_dispatchercontrol;
    void *m_zmqContext;
};

#endif // TST_BSREAD_DISPATCHERCONTROL_H
