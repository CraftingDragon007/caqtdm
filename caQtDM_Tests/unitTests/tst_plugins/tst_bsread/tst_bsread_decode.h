#ifndef TST_BSREAD_DECODE_H
#define TST_BSREAD_DECODE_H

#include <bsread_decode.h>

#include <QObject>
#include <QTest>

class Testbsread_Decode : public QObject
{
    Q_OBJECT
public:
    Testbsread_Decode() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void parsesMainHeader();
    void parsesHeader();

private:
    bsread_Decode *m_decode;
    void *m_zmqContext;
};

#endif // TST_BSREAD_DECODE_H
