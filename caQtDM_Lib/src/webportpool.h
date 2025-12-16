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
