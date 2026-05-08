#include "vncwebchildprocess.h"

#include "webportpool.h"
#include "caQtDM_Lib_global.h"

Q_LOGGING_CATEGORY(webChildProcess, "caqtdm.web.childProcess")

VncWebChildProcess::VncWebChildProcess(QObject* parent)
    : QObject(parent), m_process(nullptr), m_vncPort(30000), m_webPort(30001) {
}

VncWebChildProcess::VncWebChildProcess(quint16 vncPort, quint16 webPort, QObject* parent)
    : QObject(parent), m_process(nullptr), m_vncPort(vncPort), m_webPort(webPort) {
}

void VncWebChildProcess::setProcess(QProcess* process) {
    m_process = process;
    quint64 pid = m_process->processId();
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this, pid]() {
        QByteArray data = m_process->readAllStandardOutput();
        QTextStream stream(data);
        while (!stream.atEnd()) {
            qCInfo(webChildProcess).noquote() << QString("child (pid: %1, vnc_port: %2, web_port: %3) -- %4").arg(pid).arg(m_vncPort).arg(m_webPort).arg(stream.readLine());
        }
    });

    connect(m_process, &QProcess::readyReadStandardError, this, [this, pid]() {
        QByteArray data = m_process->readAllStandardError();
        QTextStream stream(data);
        while (!stream.atEnd()) {
            qCInfo(webChildProcess).noquote() << QString("child (pid: %1, vnc_port: %2, web_port: %3) -- %4").arg(pid).arg(m_vncPort).arg(m_webPort).arg(stream.readLine());
        }
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, pid](int exitCode, QProcess::ExitStatus exitStatus) {
        qCInfo(webChildProcess).noquote() << QString("child (pid: %1, vnc_port: %2) -- finished with exit code:").arg(pid).arg(m_vncPort) << exitCode
                 << "and exit status:" << exitStatus;
        WebPortPool::instance()->release(m_vncPort);
        WebPortPool::instance()->release(m_webPort);
    });
}
