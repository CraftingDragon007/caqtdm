#ifndef HMISHAREDEVENTBUS_H
#define HMISHAREDEVENTBUS_H

#include "hmi_common_event_defs.h"

#include <QObject>
#include <QSharedMemory>
#include <QTimer>
#include <QElapsedTimer>
#include "caQtDM_Lib_global.h"

class CAQTDM_LIBSHARED_EXPORT HmiSharedEventBus : public QObject {
    Q_OBJECT

public:
    static HmiSharedEventBus& instance();

    bool setup();
    void shutdown();
    bool isInitialized() const;

    bool sendEvent(int eventType, const QByteArray& payload = QByteArray());

signals:
    void eventReceived(int eventType, int senderPid, qint64 timestamp, const QByteArray& payload);

private slots:
    void checkForNewEvents();
    void performCleanupCheck();
    void executeCleanup();
    void onCleanupSafetyTimeout();

private:
    explicit HmiSharedEventBus(QObject *parent = nullptr);
    ~HmiSharedEventBus();

    HmiSharedEventBus(const HmiSharedEventBus&) = delete;
    HmiSharedEventBus& operator=(const HmiSharedEventBus&) = delete;
    HmiSharedEventBus(HmiSharedEventBus&&) = delete;
    HmiSharedEventBus& operator=(HmiSharedEventBus&&) = delete;

    QSharedMemory this_sharedMemory;

    SharedHeader* this_header;
    EventPayload* this_eventBuffer;

    int this_currentProcessSlotIndex;
    QTimer this_pollTimer;
    QTimer this_cleanupTimer;
    QTimer this_cleanupSafetyTimer;
    QElapsedTimer this_lastCleanupObserved;

    bool this_isInitialized;
    bool this_cleanupInProgress;
    bool this_cleanupPending;

    bool attachToSharedMemory();
    void createSharedMemory();
    int findOrCreateProcessSlot();
    void cleanupProcessSlot();
    bool validateSegmentSize() const;
    void disableBusOnLockFailure();
    void reRegisterAfterCleanup();
};
#endif // HMISHAREDEVENTBUS_H
