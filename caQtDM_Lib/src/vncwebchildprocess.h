#ifndef VNCWEBCHILDPROCESS_H
#define VNCWEBCHILDPROCESS_H

#include "qdebug.h"
#include <QObject>
#include <QProcess>
#include "webportpool.h"

class VncWebChildProcess : public QObject {
    Q_OBJECT

public:
    VncWebChildProcess(QObject* parent = nullptr) : QObject(parent), m_process(nullptr), m_vncPort(30000), m_webPort(30001) {}
    VncWebChildProcess(quint16 vncPort, quint16 webPort, QObject* parent = nullptr) : QObject(parent), m_vncPort(vncPort), m_webPort(webPort) {}

    quint16 vncPort() { return m_vncPort; }
    quint16 webPort() { return m_webPort; }
    QProcess* process() { return m_process; }

    void setVncPort(quint16 port) { m_vncPort = port; }
    void setWebPort(quint16 port) { m_webPort = port; }
    void setProcess(QProcess* process) {
        m_process = process;
        quint64 pid = m_process->processId();
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this, pid]() {
            QByteArray data = m_process->readAllStandardOutput();
            QTextStream stream(data);
            while (!stream.atEnd()) {
                qDebug().noquote() << QString("child (pid: %1, vnc_port: %2, web_port: %3) -- %4").arg(pid).arg(m_vncPort).arg(m_webPort).arg(stream.readLine());
            }
        });

        connect(m_process, &QProcess::readyReadStandardError, this, [this, pid]() {
            QByteArray data = m_process->readAllStandardError();
            QTextStream stream(data);
            while (!stream.atEnd()) {
                qDebug().noquote() << QString("child (pid: %1, vnc_port: %2, web_port: %3) -- %4").arg(pid).arg(m_vncPort).arg(m_webPort).arg(stream.readLine());
            }
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, pid](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug().noquote() << QString("child (pid: %1, vnc_port: %2) -- finished with exit code:").arg(pid).arg(m_vncPort) << exitCode
                     << "and exit status:" << exitStatus;
            WebPortPool::instance()->release(m_vncPort);
            WebPortPool::instance()->release(m_webPort);
        });
    }

private:
    QProcess* m_process;
    quint16 m_vncPort;
    quint16 m_webPort;
};

#endif // VNCWEBCHILDPROCESS_H
