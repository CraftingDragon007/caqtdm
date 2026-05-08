#ifndef VNCWEBCHILDPROCESS_H
#define VNCWEBCHILDPROCESS_H

#include <QDebug>
#include <QObject>
#include <QProcess>

class VncWebChildProcess : public QObject {
    Q_OBJECT

public:
    VncWebChildProcess(QObject* parent = nullptr);
    VncWebChildProcess(quint16 vncPort, quint16 webPort, QObject* parent = nullptr);

    quint16 vncPort() { return m_vncPort; }
    quint16 webPort() { return m_webPort; }
    QProcess* process() { return m_process; }

    void setVncPort(quint16 port) { m_vncPort = port; }
    void setWebPort(quint16 port) { m_webPort = port; }
    void setProcess(QProcess* process);

private:
    QProcess* m_process;
    quint16 m_vncPort;
    quint16 m_webPort;
};

#endif // VNCWEBCHILDPROCESS_H
