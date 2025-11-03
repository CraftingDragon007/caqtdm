#ifndef VNCWEBCHILDPROCESS_H
#define VNCWEBCHILDPROCESS_H

#include <QMetaType>
#include <QProcess>

class VncWebChildProcess {
    Q_GADGET

public:
    VncWebChildProcess() : m_process(nullptr), m_vncPort(5900), m_webPort(6900) {}
    VncWebChildProcess(quint16 vncPort, quint16 webPort, QProcess *process) : m_process(process), m_vncPort(vncPort), m_webPort(webPort) {}

    quint16 vncPort() { return m_vncPort; }
    quint16 webPort() { return m_webPort; }
    QProcess* process() { return m_process; }

    void setVncPort(quint16 port) { m_vncPort = port; }
    void setWebPort(quint16 port) { m_webPort = port; }
    void setProcess(QProcess* process) { m_process = process; }

private:
    QProcess* m_process;
    quint16 m_vncPort;
    quint16 m_webPort;
};

Q_DECLARE_METATYPE(VncWebChildProcess)

#endif // VNCWEBCHILDPROCESS_H
