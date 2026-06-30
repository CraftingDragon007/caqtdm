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
