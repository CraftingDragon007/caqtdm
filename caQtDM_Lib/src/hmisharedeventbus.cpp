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
    this_header(Q_NULLPTR),
    this_eventBuffer(Q_NULLPTR),
    this_currentProcessSlotIndex(-1),
    this_isInitialized(false) // Initially not set up
{
    this_pollTimer.setInterval(10);
    connect(&this_pollTimer, &QTimer::timeout, this, &HmiSharedEventBus::checkForNewEvents);
}

HmiSharedEventBus::~HmiSharedEventBus() {
    shutdown();
}

bool HmiSharedEventBus::setup() {
    if (this_isInitialized) {
        qCWarning(caHMILog) << "HmiSharedEventBus for PID" << QCoreApplication::applicationPid() << "is already initialized.";
        return true;
    }

    // 1. Attach to or create shared memory
    if (!attachToSharedMemory()) {
        qCCritical(caHMILog) << "Failed to set up HmiSharedEventBus: Cannot attach or create shared memory for PID" << QCoreApplication::applicationPid();
        return false;
    }

    // 2. Find or create a slot for this process in the shared header
    this_currentProcessSlotIndex = findOrCreateProcessSlot();
    if (this_currentProcessSlotIndex == -1) {
        qCCritical(caHMILog) << "Failed to find or create a process slot for PID" << QCoreApplication::applicationPid() << ". Max processes reached, or another issue.";
        this_sharedMemory.detach();
        return false;
    }

    qCInfo(caHMILog) << "Process" << QCoreApplication::applicationPid()
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
    this_isInitialized = false;

    this_pollTimer.stop();
    cleanupProcessSlot();

    if (this_sharedMemory.isAttached()) {
        this_sharedMemory.detach();
        qCInfo(caHMILog) << "Process" << QCoreApplication::applicationPid() << "detached from shared memory.";
    }
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
            qCCritical(caHMILog) << "Error attaching to shared memory for PID" << QCoreApplication::applicationPid() << ":" << this_sharedMemory.errorString();
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
    qCDebug(caHMILog) << "Process" << QCoreApplication::applicationPid() << ": Attempting to create shared memory with size:" << totalSize << "bytes";

    if (!this_sharedMemory.create(totalSize)) {
        qCCritical(caHMILog) << "Error creating shared memory segment for PID" << QCoreApplication::applicationPid() << ":" << this_sharedMemory.errorString();
        return;
    }

    if (!this_sharedMemory.isAttached()) {
        qCCritical(caHMILog) << "Shared memory created but failed to attach for PID" << QCoreApplication::applicationPid() << ". This indicates a problem.";
        return;
    }

    if (this_sharedMemory.lock()) {
        new (this_sharedMemory.data()) SharedHeader();
        this_sharedMemory.unlock();
        qCDebug(caHMILog) << "Shared memory segment created and initialized by process" << QCoreApplication::applicationPid();
    } else {
        qCCritical(caHMILog) << "Failed to lock shared memory for writing:" << this_sharedMemory.errorString();
    }
}

int HmiSharedEventBus::findOrCreateProcessSlot() {
    int currentPid = QCoreApplication::applicationPid();
    int freeSlot = -1;

    if (this_sharedMemory.lock()) {
        for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
            if (this_header->processSlots[i].pid == currentPid) {
                // This process already has a slot assigned (e.g., re-initialization or old entry).
                qCWarning(caHMILog) << "Process" << currentPid << "reusing existing slot" << i;
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
            qCDebug(caHMILog) << "Process" << currentPid << "claimed slot" << freeSlot
                              << "last read total events set to" << this_header->totalEventsWritten;
        } else {
            qCCritical(caHMILog) << "No free process slots available for PID"
                                 << currentPid << ". Max processes reached (" << MAX_PROCESS_SLOTS
                                 << ").";
        }

        this_sharedMemory.unlock();
    } else {
        qCCritical(caHMILog) << "Failed to lock shared memory for writing:" << this_sharedMemory.errorString();
    }
    return freeSlot;
}

void HmiSharedEventBus::cleanupProcessSlot() {
    // Only proceed if the bus was successfully initialized and we have a valid header/slot.
    if (this_isInitialized && this_currentProcessSlotIndex != -1 && this_header) {
        if (this_sharedMemory.lock()) {
            // Double-check if our PID is still in the slot before clearing.
            if (this_header->processSlots[this_currentProcessSlotIndex].pid
                == QCoreApplication::applicationPid()) {
                this_header->processSlots[this_currentProcessSlotIndex].pid = 0; // Mark slot as free
                this_header->processSlots[this_currentProcessSlotIndex].lastReadTotalEvents
                    = 0; // Reset
                qCDebug(caHMILog) << "Process" << QCoreApplication::applicationPid()
                                  << "released slot" << this_currentProcessSlotIndex;
            }
            this_sharedMemory.unlock();
        } else {
            qCCritical(caHMILog) << "Failed to lock shared memory for writing:" << this_sharedMemory.errorString();
        }
    }
}

bool HmiSharedEventBus::sendEvent(int eventType, const QByteArray& payload) {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header) {
        qCWarning(caHMILog) << "HmiSharedEventBus not initialized. Cannot send event for PID" << QCoreApplication::applicationPid();
        return false;
    }

    if (payload.size() > EVENT_PAYLOAD_SIZE) {
        qCWarning(caHMILog) << "Event payload size (" << payload.size()
        << ") exceeds max allowed (" << EVENT_PAYLOAD_SIZE << "). Event truncated or ignored by PID" << QCoreApplication::applicationPid();
        // Payload size to large
        return false;
    }

    if (!this_sharedMemory.lock()) {
        qCCritical(caHMILog) << "Unable to lock sharedMemory, error:" << this_sharedMemory.errorString();
        qCCritical(caHMILog) << "To prevent crashes HmiShharedEventBus is now disabled!";
#ifdef linux
        qCCritical(caHMILog) << "Please clean up /tmp/*caQtDM* and restart the program if you want to use caHMIConfig features";
#endif
        this_isInitialized = false;
        this_pollTimer.stop();
        return false;
    }

    quint32 writeIndex = this_header->currentWriteIndex % EVENT_BUFFER_CAPACITY;

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

    this_header->currentWriteIndex = (writeIndex + 1) % EVENT_BUFFER_CAPACITY;
    this_header->totalEventsWritten++;

    this_sharedMemory.unlock();

    qCDebug(caHMILog) << "Process" << QCoreApplication::applicationPid()
             << "sent event" << EventTypes(eventType) << "at index" << writeIndex
             << "Total events written:" << this_header->totalEventsWritten;

    return true;
}

void HmiSharedEventBus::checkForNewEvents() {
    if (!this_isInitialized || this_currentProcessSlotIndex == -1 || !this_header) {
        // Not fully initialized, or header not mapped yet.
        return;
    }

    // Snapshot global and local counters to ensure consistency during processing.
    if (!this_sharedMemory.lock()) {
        qCCritical(caHMILog) << "Unable to lock sharedMemory, error:" << this_sharedMemory.errorString();
        return;
    }

    quint64 currentTotalEvents = this_header->totalEventsWritten;
    quint64& lastReadTotalEvents = this_header->processSlots[this_currentProcessSlotIndex].lastReadTotalEvents;
    quint32 currentWriteIndex = this_header->currentWriteIndex;
    quint64 unreadGlobalEvents = currentTotalEvents - lastReadTotalEvents;
    lastReadTotalEvents = currentTotalEvents;

    this_sharedMemory.unlock();

    if (unreadGlobalEvents == 0) {
        return; // No new events available
    }

    quint64 eventsToProcess = std::min(unreadGlobalEvents, (quint64)EVENT_BUFFER_CAPACITY);
    quint32 readBufferStartIndex = (currentWriteIndex - eventsToProcess + EVENT_BUFFER_CAPACITY * 2) % EVENT_BUFFER_CAPACITY;

    // Reading here happens without lockingm, since locking with multiple processes every 5ms can quickly slow down performance drastically
    // It may be that events are overwritten during our reads, but that might also happen during the poll timeout.
    for (quint64 i = 0; i < eventsToProcess; ++i) {
        quint32 eventBufferIndex = (readBufferStartIndex + i) % EVENT_BUFFER_CAPACITY;
        const EventPayload& event = this_eventBuffer[eventBufferIndex];

        QByteArray payloadData;
        int dataSize = event.dataSize;
        if (dataSize > 0 && dataSize <= EVENT_PAYLOAD_SIZE) {
            payloadData = QByteArray(event.data, dataSize);
        }

        emit eventReceived(event.eventType, event.senderPid, event.timestamp, payloadData);
    }
}
