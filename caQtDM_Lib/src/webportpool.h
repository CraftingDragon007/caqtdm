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

#ifndef WEBPORTPOOL_H
#define WEBPORTPOOL_H
#include <QObject>
#include <QSet>
#include <QMutex>
#include <QHostAddress>
#include <QTcpSocket>
#include "caQtDM_Lib_global.h"

class CAQTDM_LIBSHARED_EXPORT WebPortPool : public QObject
{
    Q_OBJECT
public:
    /**
     * @return Singleton instance (lazy-init, thread-safe, app-lifetime).
     * Usage: WebPortPool::instance()->allocate(port1, port2);
     */
    static WebPortPool* instance();

    /**
     * Allocates two unique free ports from the pool
     * Verifies both are bindable on 127.0.0.1.
     * @return true if successful, ports set via out-params; false if pool exhausted or bind fails (rollback).
     */
    bool allocate(quint16& port1, quint16& port2);

    /**
     * Releases a single port back to the pool.
     */
    void release(quint16 port);

    /**
     * @return Number of free ports available.
     */
    quint16 freeCount() const;

signals:
    /**
     * Emitted when free ports drop below 100 (warning threshold).
     */
    void lowPortsAvailable(int remaining);

private:
    explicit WebPortPool(QObject *parent = nullptr);
    ~WebPortPool() = default;

    bool isPortFree(const QHostAddress& addr, quint16 port) const;

    mutable QMutex m_mutex;
    QSet<quint16> m_freePorts;

    static constexpr quint16 BASE_PORT = 30000;
    static constexpr quint16 POOL_SIZE = 10000;
};
#endif // WEBPORTPOOL_H
