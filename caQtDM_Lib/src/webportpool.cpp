#include "webportpool.h"
#include <QMutexLocker>
#include <QDebug>
#include <QAbstractSocket>

WebPortPool::WebPortPool(QObject *parent)
    : QObject{parent}
{
    QMutexLocker lock(&m_mutex);
    quint16 port = BASE_PORT;
    for (quint16 i = 0; i < POOL_SIZE; ++i, ++port) {
        m_freePorts.insert(port);
    }
    if (m_freePorts.size() < POOL_SIZE) {
        qWarning() << "WebPortPool: Initialized only" << m_freePorts.size() << "ports (range limited to 65535)";
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

    if (m_freePorts.isEmpty()) {
        return false;
    }

    auto it1 = m_freePorts.begin();
    port1 = *it1;
    m_freePorts.erase(it1);

    if (m_freePorts.isEmpty()) {
        m_freePorts.insert(port1);  // Rollback
        return false;
    }

    auto it2 = m_freePorts.begin();
    port2 = *it2;
    m_freePorts.erase(it2);

    // Strict bind-check on localhost
    if (!isPortFree(QHostAddress("127.0.0.1"), port1) || !isPortFree(QHostAddress("127.0.0.1"), port2)) {
        m_freePorts.insert(port1);
        m_freePorts.insert(port2);
        return false;
    }

    if (m_freePorts.size() < 100) {
        emit lowPortsAvailable(static_cast<int>(m_freePorts.size()));
    }

    return true;
}

void WebPortPool::release(quint16 port)
{
    if (port < BASE_PORT) {
        qWarning() << "Invalid port released:" << port;
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
