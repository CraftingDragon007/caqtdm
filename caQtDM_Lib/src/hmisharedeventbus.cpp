#include <QDebug>
#include <QCoreApplication>
#include <QVector>
#include "hmisharedeventbus.h"
#include "causerid.h"

HmiSharedEventBus& HmiSharedEventBus::instance() {
    static HmiSharedEventBus bus;
    return bus;
}

HmiSharedEventBus::HmiSharedEventBus(QObject *parent)
    : QObject(parent),
    this_sharedMemory(QString(SHARED_MEM_KEY).arg(getUniqueUserId())),
    this_header(nullptr),
    this_eventBuffer(nullptr),
    this_currentProcessSlotIndex(-1),
    this_isInitialized(false), // Initially not set up
    this_cleanupInProgress(false),
    this_cleanupPending(false)
{
    this_pollTimer.setInterval(50);
    connect(&this_pollTimer, &QTimer::timeout, this, &HmiSharedEventBus::checkForNewEvents);

    this_cleanupTimer.setInterval(CLEANUP_INTERVAL_MS);
    connect(&this_cleanupTimer, &QTimer::timeout, this, &HmiSharedEventBus::performCleanupCheck);
    this_cleanupSafetyTimer.setSingleShot(true);
    this_cleanupSafetyTimer.setInterval(CLEANUP_SAFETY_TIMEOUT_MS);
    connect(&this_cleanupSafetyTimer, &QTimer::timeout, this, &HmiSharedEventBus::onCleanupSafetyTimeout);
}

HmiSharedEventBus::~HmiSharedEventBus() {
    shutdown();
}

bool HmiSharedEventBus::setup() {
    if (this_isInitialized) {
        qWarning() << PREFIX << "HmiSharedEventBus for PID" << QCoreApplication::applicationPid() << "is already initialized.";
        return true;
    }

    // 1. Attach to or create shared memory
    if (!attachToSharedMemory()) {
        qCritical() << PREFIX << "Failed to set up HmiSharedEventBus: Cannot attach or create shared memory for PID" << QCoreApplication::applicationPid();
        return false;
    }

    // 2. Find or create a slot for this process in the shared header
    this_currentProcessSlotIndex = findOrCreateProcessSlot();
    if (this_currentProcessSlotIndex == -1) {
        qCritical() << PREFIX << "Failed to find or create a process slot for PID" << QCoreApplication::applicationPid() << ". Max processes reached, or another issue.";
        this_sharedMemory.detach();
        return false;
    }

    qDebug() << PREFIX << "Process" << QCoreApplication::applicationPid()
             << "initialized in slot" << this_currentProcessSlotIndex;

    // 3. Start polling for new events
    this_pollTimer.start();
    this_cleanupTimer.start();
    this_lastCleanupObserved.start();
    this_isInitialized = true;
    return true;
}

void HmiSharedEventBus::shutdown() {
    if (!this_isInitialized) {
        return;
    }
    this_pollTimer.stop();
    this_cleanupTimer.stop();
    this_cleanupSafetyTimer.stop();
    this_cleanupInProgress = false;
    this_cleanupPending = false;
    cleanupProcessSlot();

    if (this_sharedMemory.isAttached()) {
        this_sharedMemory.detach();
        qDebug() << PREFIX << "Process" << QCoreApplication::applicationPid() << "detached from shared memory.";
    }
    this_isInitialized = false;
}

bool HmiSharedEventBus::isInitialized() const {
    return this_isInitialized;
}

bool HmiSharedEventBus::attachToSharedMemory() {
    if (!this_sharedMemory.attach()) {
        if (this_sharedMemory.error() == QSharedMemory::NotFound) {
            createSharedMemory();
            if (!this_sharedMemory.isAttached()) {
                return false;
            }
        } else {
            qCritical() << PREFIX << "Error attaching to shared memory for PID" << QCoreApplication::applicationPid() << ":" << this_sharedMemory.errorString();
            return false;
        }
    }

    // Reject segments that are too small for our layout (e.g. stale leftovers)
    if (!validateSegmentSize()) {
        qCritical() << PREFIX << "Shared memory segment has unexpected size" << this_sharedMemory.size()
                    << "- HMI event bus disabled for PID" << QCoreApplication::applicationPid();
#ifdef linux
        qCritical() << PREFIX << "Please clean up /tmp/*caQtDM* and stale ipcs -m / ipcs -s entries, then restart.";
#endif
        this_sharedMemory.detach();
        return false;
    }

    // Once attached (or created and attached), map the pointers to the data.
    this_header = static_cast<SharedHeader*>(this_sharedMemory.data());
    this_eventBuffer = reinterpret_cast<EventPayload*>(
        static_cast<char*>(this_sharedMemory.data()) + sizeof(SharedHeader)
        );

    return true;
}

bool HmiSharedEventBus::validateSegmentSize() const {
    // ">=" because Qt may round the size up to a page multiple
    size_t expected = sizeof(SharedHeader) + EVENT_BUFFER_CAPACITY * sizeof(EventPayload);
    return static_cast<size_t>(this_sharedMemory.size()) >= expected;
}

void HmiSharedEventBus::disableBusOnLockFailure() {
    qCritical() << PREFIX << "Unable to lock shared memory, error:" << this_sharedMemory.errorString();
    qCritical() << PREFIX << "To prevent crashes HmiSharedEventBus is now disabled!";
#ifdef linux
    qCritical() << PREFIX << "Please clean up /tmp/*caQtDM* and restart the program if you want to use caHMIConfig features";
#endif
    this_isInitialized = false;
    this_pollTimer.stop();
    this_cleanupTimer.stop();
    this_cleanupSafetyTimer.stop();
    this_cleanupInProgress = false;
    this_cleanupPending = false;
}

void HmiSharedEventBus::performCleanupCheck() {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header) return;
    if (this_cleanupInProgress || this_cleanupPending) return; // a cleanup is already running or scheduled

    if (!this_sharedMemory.lock()) {
        disableBusOnLockFailure();
        return;
    }
    // Elect: lowest registered pid wins, no OS liveness check needed
    qint64 electedPid = 0;
    for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
        int pid = this_header->processSlots[i].pid;
        if (pid != 0 && (electedPid == 0 || pid < electedPid)) electedPid = pid;
    }
    this_sharedMemory.unlock();

    // Fallback: if the elected pid is dead and never cleans, take over after 2 idle intervals.
    // Staggered per slot so concurrent takeovers stay rare; reading a Started resets the clock.
    qint64 stallThreshold = 2 * (qint64)CLEANUP_INTERVAL_MS
                            + (qint64)this_currentProcessSlotIndex * 4 * CLEANUP_GRACE_MS;
    bool cleanupStalled = this_lastCleanupObserved.isValid()
                          && this_lastCleanupObserved.elapsed() > stallThreshold;
    if (electedPid != QCoreApplication::applicationPid() && !cleanupStalled) return;

    if (!sendEvent(EventTypes::CleanupStarted)) return; // abort this round
    this_cleanupPending = true;
    // Grace period so peers poll CleanupStarted before the ring is wiped
    QTimer::singleShot(CLEANUP_GRACE_MS, this, &HmiSharedEventBus::executeCleanup);
}

void HmiSharedEventBus::executeCleanup() {
    this_cleanupPending = false;
    if (!this_isInitialized || !this_header || !this_eventBuffer) return;

    if (!this_sharedMemory.lock()) {
        disableBusOnLockFailure();
        return;
    }
    // Wipe everything; liveness is proven by re-registration after CleanupFinished
    int clearedSlots = 0;
    for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
        if (this_header->processSlots[i].pid != 0) ++clearedSlots;
        this_header->processSlots[i].pid = 0;
        this_header->processSlots[i].lastReadTotalEvents = 0;
    }
    std::memset(this_eventBuffer, 0, (size_t)EVENT_BUFFER_CAPACITY * sizeof(EventPayload));
    this_header->currentWriteIndex = 0;
    this_header->totalEventsWritten = 0;
    // Re-register the cleaner itself under the same lock (lock is not recursive)
    this_header->processSlots[0].pid = QCoreApplication::applicationPid();
    this_header->processSlots[0].lastReadTotalEvents = 0;
    this_currentProcessSlotIndex = 0;
    this_sharedMemory.unlock();

    this_lastCleanupObserved.restart();
    // Our wipe supersedes any concurrent cleanup - never stay paused after cleaning
    this_cleanupInProgress = false;
    this_cleanupSafetyTimer.stop();
    sendEvent(EventTypes::CleanupFinished);
    qDebug() << PREFIX << "Cleanup done by PID" << QCoreApplication::applicationPid() << "- cleared" << clearedSlots << "slots";
}

void HmiSharedEventBus::reRegisterAfterCleanup() {
    this_currentProcessSlotIndex = findOrCreateProcessSlot();
    if (this_currentProcessSlotIndex == -1) {
        qCritical() << PREFIX << "Re-registration after cleanup failed for PID" << QCoreApplication::applicationPid() << "- bus disabled.";
        this_isInitialized = false;
        this_pollTimer.stop();
        this_cleanupTimer.stop();
    }
}

void HmiSharedEventBus::onCleanupSafetyTimeout() {
    if (!this_cleanupInProgress) return;
    qWarning() << PREFIX << "CleanupFinished never arrived, resuming normal operation.( PID: "<< QCoreApplication::applicationPid()<< ")";
    this_cleanupInProgress = false;
    reRegisterAfterCleanup();
}

void HmiSharedEventBus::createSharedMemory() {
    size_t totalSize = sizeof(SharedHeader) + EVENT_BUFFER_CAPACITY * sizeof(EventPayload);
    qDebug() << PREFIX << "Process" << QCoreApplication::applicationPid() << ": Attempting to create shared memory with size:" << totalSize << "bytes";

    if (!this_sharedMemory.create(totalSize)) {
        qCritical() << PREFIX << "Error creating shared memory segment for PID" << QCoreApplication::applicationPid() << ":" << this_sharedMemory.errorString();
        return;
    }

    if (!this_sharedMemory.isAttached()) {
        qCritical() << PREFIX << "Shared memory created but failed to attach for PID" << QCoreApplication::applicationPid() << ". This indicates a problem.";
        return;
    }

    if (this_sharedMemory.lock()) {
        new (this_sharedMemory.data()) SharedHeader();
        this_sharedMemory.unlock();
    } else {
        qCritical() << PREFIX << "Failed to lock shared memory for initialization:" << this_sharedMemory.errorString();
    }

    qDebug() << PREFIX << "Shared memory segment created and initialized by process" << QCoreApplication::applicationPid();
}

int HmiSharedEventBus::findOrCreateProcessSlot() {
    int currentPid = QCoreApplication::applicationPid();

    if (!this_sharedMemory.lock()) {
        qCritical() << PREFIX << "Failed to lock shared memory for slot registration:" << this_sharedMemory.errorString();
        return -1;
    }

    int freeSlot = -1;
    for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
        if (this_header->processSlots[i].pid == currentPid) {
            // This process already has a slot assigned (e.g., re-initialization or old entry).
            qWarning() << PREFIX << "Process" << currentPid << "reusing existing slot" << i;
            this_sharedMemory.unlock();
            return i;
        }
        if (this_header->processSlots[i].pid == 0 && freeSlot == -1) {
            freeSlot = i; // Remember the first free slot we find.
        }
    }

    if (freeSlot != -1) {
        this_header->processSlots[freeSlot].pid = currentPid;
        // Initialize this slot's last read index to the current total events written.
        // This ensures the process only sees events *after* it initialized.
        this_header->processSlots[freeSlot].lastReadTotalEvents = this_header->totalEventsWritten;
//        qDebug() << PREFIX << "Process" << currentPid << "claimed slot" << freeSlot
//                 << "last read total events set to" << this_header->totalEventsWritten;
    } else {
        qCritical() << PREFIX << "No free process slots available for PID" << currentPid << ". Max processes reached (" << MAX_PROCESS_SLOTS << ").";
    }

    this_sharedMemory.unlock(); // Release the lock.
    return freeSlot;
}

void HmiSharedEventBus::cleanupProcessSlot() {
    // Only proceed if the bus was successfully initialized and we have a valid header/slot.
    if (this_isInitialized && this_currentProcessSlotIndex != -1 && this_header) {
        if (!this_sharedMemory.lock()) {
            qCritical() << PREFIX << "Failed to lock shared memory for slot release:" << this_sharedMemory.errorString();
            return;
        }
        // Double-check if our PID is still in the slot before clearing.
        if (this_header->processSlots[this_currentProcessSlotIndex].pid == QCoreApplication::applicationPid()) {
            this_header->processSlots[this_currentProcessSlotIndex].pid = 0; // Mark slot as free
            this_header->processSlots[this_currentProcessSlotIndex].lastReadTotalEvents = 0; // Reset
            // qDebug() << PREFIX << "Process" << QCoreApplication::applicationPid() << "released slot" << this_currentProcessSlotIndex;
        }
        this_sharedMemory.unlock();
    }
}

bool HmiSharedEventBus::sendEvent(int eventType, const QByteArray& payload) {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header) {
        qWarning() << PREFIX << "HmiSharedEventBus not initialized. Cannot send event for PID" << QCoreApplication::applicationPid();
        return false;
    }

    if (this_cleanupInProgress && eventType != EventTypes::CleanupStarted && eventType != EventTypes::CleanupFinished) {
        return false; // dropped silently during cleanup
    }

    if (payload.size() > EVENT_PAYLOAD_SIZE) {
        qWarning() << PREFIX << "Event payload size (" << payload.size()
        << ") exceeds max allowed (" << EVENT_PAYLOAD_SIZE << "). Event truncated or ignored by PID" << QCoreApplication::applicationPid();
        // Payload size to large
        return false;
    }

    if (eventType <= EventTypes::Invalid || eventType > LAST_EVENT_TYPE) {
        qWarning() << PREFIX << "Refusing to send event with invalid type" << eventType;
        return false;
    }

    if (!this_sharedMemory.lock()) {
        disableBusOnLockFailure();
        return false;
    }

    quint32 writeIndex = this_header->currentWriteIndex % EVENT_BUFFER_CAPACITY;
    if (!this_eventBuffer) {
        qCritical() << PREFIX << "this_eventBuffer is a null pointer.";
        this_sharedMemory.unlock();
        return false;
    }
    EventPayload& event = this_eventBuffer[writeIndex];
    event.eventType = eventType;
    event.senderPid = QCoreApplication::applicationPid();
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.dataSize = payload.size();
    if (!payload.isEmpty()) {
        std::memcpy(event.data, payload.constData(), payload.size());
        // Clear stale bytes behind the payload
        std::memset(event.data + payload.size(), 0, EVENT_PAYLOAD_SIZE - payload.size());
    } else {
        std::memset(event.data, 0, EVENT_PAYLOAD_SIZE); // Clear if no payload.
    }

    this_header->currentWriteIndex = (writeIndex + 1) % EVENT_BUFFER_CAPACITY;
    this_header->totalEventsWritten++;

    this_sharedMemory.unlock();

    /*
    qDebug() << "Process" << QCoreApplication::applicationPid()
             << "sent event" << EventTypes(eventType) << "at index" << writeIndex
             << "Total events written:" << this_header->totalEventsWritten; */

    return true;
}

void HmiSharedEventBus::checkForNewEvents() {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header || !this_eventBuffer) {
        // Not fully initialized, or header not mapped yet.
        return;
    }

    struct PendingEvent {
        int eventType;
        int senderPid;
        qint64 timestamp;
        QByteArray payload;
    };
    QVector<PendingEvent> pending;
    bool needReRegister = false;

    // copy under lock, emit after unlock (lock is not recursive)
    if (!this_sharedMemory.lock()) {
        disableBusOnLockFailure();
        return;
    }

    // Slot no longer ours -> a cleanup wiped the table: re-register and resume.
    // CleanupFinished may be invisible here (old slot index reused by another process).
    if (this_header->processSlots[this_currentProcessSlotIndex].pid != QCoreApplication::applicationPid()) {
        this_sharedMemory.unlock();
        this_lastCleanupObserved.restart();
        this_cleanupInProgress = false;
        this_cleanupSafetyTimer.stop();
        reRegisterAfterCleanup();
        return;
    }

    quint64 currentTotalEvents = this_header->totalEventsWritten;
    quint64& lastReadTotalEvents = this_header->processSlots[this_currentProcessSlotIndex].lastReadTotalEvents;

    if (lastReadTotalEvents > currentTotalEvents) {
        // Corrupted or reset counters: resync instead of underflowing
        qWarning() << PREFIX << "Event counters inconsistent (" << lastReadTotalEvents << ">" << currentTotalEvents << "), resyncing.";
        lastReadTotalEvents = currentTotalEvents;
        this_sharedMemory.unlock();
        return;
    }

    quint64 unreadGlobalEvents = currentTotalEvents - lastReadTotalEvents;
    if (unreadGlobalEvents == 0) {
        this_sharedMemory.unlock();
        return; // No new events available
    }

    quint64 eventsToProcess = std::min(unreadGlobalEvents, (quint64)EVENT_BUFFER_CAPACITY);
    quint32 readBufferStartIndex = (this_header->currentWriteIndex - eventsToProcess + EVENT_BUFFER_CAPACITY * 2) % EVENT_BUFFER_CAPACITY;

    pending.reserve((int)eventsToProcess);
    for (quint64 i = 0; i < eventsToProcess; ++i) {
        quint32 eventBufferIndex = (readBufferStartIndex + i) % EVENT_BUFFER_CAPACITY;
        const EventPayload& event = this_eventBuffer[eventBufferIndex];

        // Skip implausible slots
        if (event.eventType <= EventTypes::Invalid || event.eventType > LAST_EVENT_TYPE) continue;
        if (event.dataSize < 0 || event.dataSize > EVENT_PAYLOAD_SIZE) continue;
        if (event.senderPid <= 0) continue;

        if (event.eventType == EventTypes::CleanupStarted || event.eventType == EventTypes::CleanupFinished) {
            this_lastCleanupObserved.restart();
            if (event.senderPid != QCoreApplication::applicationPid()) {
                if (event.eventType == EventTypes::CleanupStarted) {
                    this_cleanupInProgress = true;
                    this_cleanupSafetyTimer.start();
                } else {
                    this_cleanupInProgress = false;
                    this_cleanupSafetyTimer.stop();
                    needReRegister = true; // our slot was wiped, claim a fresh one after unlock
                }
            }
            continue; // control events stay bus-internal
        }
        if (this_cleanupInProgress) continue; // paused: drop payload events

        PendingEvent p;
        p.eventType = event.eventType;
        p.senderPid = event.senderPid;
        p.timestamp = event.timestamp;
        if (event.dataSize > 0) {
            p.payload = QByteArray(event.data, event.dataSize);
        }
        pending.append(p);
    }

    lastReadTotalEvents = currentTotalEvents;
    this_sharedMemory.unlock();

    if (needReRegister) reRegisterAfterCleanup(); // needs the lock, so after unlock

    for (int i = 0; i < pending.size(); ++i) {
        emit eventReceived(pending.at(i).eventType, pending.at(i).senderPid, pending.at(i).timestamp, pending.at(i).payload);
    }
}
