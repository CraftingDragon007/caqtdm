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

#include "webportpool.h"
#include <QMutexLocker>
#include <QDebug>
#include <QAbstractSocket>

Q_LOGGING_CATEGORY(webPortPool, "caqtdm.web.portPool")

WebPortPool::WebPortPool(QObject *parent)
    : QObject{parent}
{
    QMutexLocker lock(&m_mutex);
    quint16 port = BASE_PORT;
    for (quint16 i = 0; i < POOL_SIZE; ++i, ++port) {
        m_freePorts.insert(port);
    }
    if (m_freePorts.size() < POOL_SIZE) {
        qCWarning(webPortPool) << "WebPortPool: Initialized only" << m_freePorts.size() << "ports (range limited to 65535)";
    }
}

WebPortPool* WebPortPool::instance()
{
    static WebPortPool pool(nullptr);
    return &pool;
}

bool WebPortPool::allocate(quint16& port1, quint16& port2)
{
    QMutexLocker lock(&m_mutex);

    while (m_freePorts.size() >= 2) {
        auto it1 = m_freePorts.begin();
        quint16 p1 = *it1;
        m_freePorts.erase(it1);

        if (!isPortFree(QHostAddress("127.0.0.1"), p1)) {
            continue;
        }

        auto it2 = m_freePorts.begin();
        quint16 p2 = *it2;
        m_freePorts.erase(it2);

        if (!isPortFree(QHostAddress("127.0.0.1"), p2)) {
            m_freePorts.insert(p1);
            continue;
        }

        port1 = p1;
        port2 = p2;

        if (m_freePorts.size() < 100) {
            emit lowPortsAvailable(static_cast<int>(m_freePorts.size()));
        }
        return true;
    }

    return false;
}

void WebPortPool::release(quint16 port)
{
    if (port < BASE_PORT) {
        qCWarning(webPortPool) << "Invalid port released:" << port;
        return;
    }

    QMutexLocker lock(&m_mutex);
    m_freePorts.insert(port);
}

quint16 WebPortPool::freeCount() const
{
    QMutexLocker lock(&m_mutex);
    return static_cast<quint16>(m_freePorts.size());
}

bool WebPortPool::isPortFree(const QHostAddress& addr, quint16 port) const
{
    QTcpSocket socket;
    if (!socket.bind(addr, port, QAbstractSocket::DontShareAddress)) {
        return false;
    }
    socket.close();
    return true;
}
