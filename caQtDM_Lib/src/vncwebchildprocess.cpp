/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2026
 *
 *  Author:
 *    Julian Houba
 *  Contact details:
 *    julian.houba@psi.ch
 */

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
