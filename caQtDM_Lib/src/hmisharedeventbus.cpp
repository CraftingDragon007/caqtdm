#include <QDebug>
#include <QCoreApplication>
#include "hmisharedeventbus.h"
#include "causerid.h"

HmiSharedEventBus& HmiSharedEventBus::instance() {
    static HmiSharedEventBus bus;
    return bus;
}

HmiSharedEventBus::HmiSharedEventBus(QObject *parent)
    : QObject(parent),
    this_sharedMemory(QString(SHARED_MEM_KEY).arg(getUniqueUserId())),
    // Initialize semaphore with 1 (available) for mutual exclusion (mutex behavior)
    this_writeLockSemaphore(QString(WRITE_LOCK_SEM_KEY).arg(getUniqueUserId()), 1, QSystemSemaphore::Open),
    this_header(nullptr),
    this_eventBuffer(nullptr),
    this_currentProcessSlotIndex(-1),
    this_isInitialized(false) // Initially not set up
{
    this_pollTimer.setInterval(50);
    connect(&this_pollTimer, &QTimer::timeout, this, &HmiSharedEventBus::checkForNewEvents);
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
    this_isInitialized = true;
    return true;
}

void HmiSharedEventBus::shutdown() {
    if (!this_isInitialized) {
        return;
    }
    this_pollTimer.stop();
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

    // Once attached (or created and attached), map the pointers to the data.
    this_header = static_cast<SharedHeader*>(this_sharedMemory.data());
    this_eventBuffer = reinterpret_cast<EventPayload*>(
        static_cast<char*>(this_sharedMemory.data()) + sizeof(SharedHeader)
        );

    return true;
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

    this_writeLockSemaphore.acquire();
    new (this_sharedMemory.data()) SharedHeader();
    this_writeLockSemaphore.release();

    qDebug() << PREFIX << "Shared memory segment created and initialized by process" << QCoreApplication::applicationPid();
}

int HmiSharedEventBus::findOrCreateProcessSlot() {
    int currentPid = QCoreApplication::applicationPid();

    this_writeLockSemaphore.acquire();

    int freeSlot = -1;
    for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
        if (this_header->processSlots[i].pid == currentPid) {
            // This process already has a slot assigned (e.g., re-initialization or old entry).
            qWarning() << PREFIX << "Process" << currentPid << "reusing existing slot" << i;
            this_writeLockSemaphore.release();
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
        qDebug() << PREFIX << "Process" << currentPid << "claimed slot" << freeSlot
                 << "last read total events set to" << this_header->totalEventsWritten;
    } else {
        qCritical() << PREFIX << "No free process slots available for PID" << currentPid << ". Max processes reached (" << MAX_PROCESS_SLOTS << ").";
    }

    this_writeLockSemaphore.release(); // Release the lock.
    return freeSlot;
}

void HmiSharedEventBus::cleanupProcessSlot() {
    // Only proceed if the bus was successfully initialized and we have a valid header/slot.
    if (this_isInitialized && this_currentProcessSlotIndex != -1 && this_header) {
        this_writeLockSemaphore.acquire();
        // Double-check if our PID is still in the slot before clearing.
        if (this_header->processSlots[this_currentProcessSlotIndex].pid == QCoreApplication::applicationPid()) {
            this_header->processSlots[this_currentProcessSlotIndex].pid = 0; // Mark slot as free
            this_header->processSlots[this_currentProcessSlotIndex].lastReadTotalEvents = 0; // Reset
            qDebug() << PREFIX << "Process" << QCoreApplication::applicationPid() << "released slot" << this_currentProcessSlotIndex;
        }
        this_writeLockSemaphore.release();
    }
}

bool HmiSharedEventBus::sendEvent(int eventType, const QByteArray& payload) {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header) {
        qWarning() << PREFIX << "HmiSharedEventBus not initialized. Cannot send event for PID" << QCoreApplication::applicationPid();
        return false;
    }

    if (payload.size() > EVENT_PAYLOAD_SIZE) {
        qWarning() << PREFIX << "Event payload size (" << payload.size()
        << ") exceeds max allowed (" << EVENT_PAYLOAD_SIZE << "). Event truncated or ignored by PID" << QCoreApplication::applicationPid();
        // Payload size to large
        return false;
    }

    this_writeLockSemaphore.acquire();

    int writeIndex = this_header->currentWriteIndex;
    int nextWriteIndex = (writeIndex + 1) % EVENT_BUFFER_CAPACITY;

    EventPayload& event = this_eventBuffer[writeIndex];
    event.eventType = eventType;
    event.senderPid = QCoreApplication::applicationPid();
    event.timestamp = QDateTime::currentMSecsSinceEpoch();
    event.dataSize = payload.size();
    if (!payload.isEmpty()) {
        std::memcpy(event.data, payload.constData(), payload.size());
    } else {
        std::memset(event.data, 0, EVENT_PAYLOAD_SIZE); // Clear if no payload.
    }

    this_header->currentWriteIndex = nextWriteIndex;
    this_header->totalEventsWritten++;

    this_writeLockSemaphore.release();

    /*
    qDebug() << "Process" << QCoreApplication::applicationPid()
             << "sent event" << EventTypes(eventType) << "at index" << writeIndex
             << "Total events written:" << this_header->totalEventsWritten; */

    return true;
}

void HmiSharedEventBus::checkForNewEvents() {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header) {
        // Not fully initialized, or header not mapped yet.
        return;
    }

    // Snapshot global and local counters to ensure consistency during processing.
    quint64 currentTotalEvents = this_header->totalEventsWritten;
    quint64& lastReadTotalEvents = this_header->processSlots[this_currentProcessSlotIndex].lastReadTotalEvents;

    quint64 unreadGlobalEvents = currentTotalEvents - lastReadTotalEvents;

    if (unreadGlobalEvents == 0) {
        return; // No new events available
    }

    quint64 eventsToProcess = std::min(unreadGlobalEvents, (quint64)EVENT_BUFFER_CAPACITY);

    int readBufferStartIndex = (this_header->currentWriteIndex - eventsToProcess + EVENT_BUFFER_CAPACITY * 2) % EVENT_BUFFER_CAPACITY;


    for (quint64 i = 0; i < eventsToProcess; ++i) {
        int eventBufferIndex = (readBufferStartIndex + i) % EVENT_BUFFER_CAPACITY;
        const EventPayload& event = this_eventBuffer[eventBufferIndex];

        QByteArray payloadData;
        if (event.dataSize > 0) {
            payloadData = QByteArray(event.data, event.dataSize);
        }

        emit eventReceived(event.eventType, event.senderPid, event.timestamp, payloadData);
    }

    lastReadTotalEvents = currentTotalEvents;
}
