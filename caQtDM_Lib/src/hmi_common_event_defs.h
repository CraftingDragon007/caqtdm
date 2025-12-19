#ifndef HMI_COMMON_EVENT_DEFS_H
#define HMI_COMMON_EVENT_DEFS_H

#include <QDateTime>
#include <QByteArray>
#include <cstring>

// Unique keys for shared memory and semaphore
#define SHARED_MEM_KEY "caQtDM_HmiSharedEventBus_SharedMem_%1"
#define WRITE_LOCK_SEM_KEY "caQtDM_HmiSharedEventBus_WriteLock_%1"

// Configuration parameters
#define MAX_PROCESS_SLOTS 5000      // Maximum number of concurrent processes
#define EVENT_PAYLOAD_SIZE 8192    // Fixed size for event data payload
#define EVENT_BUFFER_CAPACITY 512 // Max events in the ring buffer

#define PREFIX "HmiSharedEventBus"

enum EventTypes {
    Invalid = 0,
    NewCaHMIConfig,
    CaHMIConfigDeleted,
    CaHMIConfigEnabledChanged,
    KeyPress,
    MouseMove,
    MousePress
};

// Structure of an event
struct EventPayload {
    int eventType;
    int senderPid; // Process ID of the sender
    qint64 timestamp;
    char data[EVENT_PAYLOAD_SIZE];
    int dataSize; // size of data in payload, up to EVENT_PAYLOAD_SIZE

    EventPayload() : eventType(0), senderPid(0), timestamp(0), dataSize(0) {
        std::memset(data, 0, EVENT_PAYLOAD_SIZE);
    }
};

struct SharedHeader {
    int currentWriteIndex;  // Next available slot for writing (0 to EVENT_BUFFER_CAPACITY - 1)
    quint64 totalEventsWritten;

    // Each slot represents a potential active process.
    // pid = 0 means slot is free.
    struct ProcessSlot {
        int pid;                 // Process ID of the active process in this slot (0 if free)
        quint64 lastReadTotalEvents; // Amount of events the process has read
    } processSlots[MAX_PROCESS_SLOTS];

    SharedHeader() : currentWriteIndex(0), totalEventsWritten(0) {
        for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
            processSlots[i].pid = 0;
            processSlots[i].lastReadTotalEvents = 0;
        }
    }
};

// Total size of shared memory needed:
// sizeof(SharedHeader) + EVENT_BUFFER_CAPACITY * sizeof(EventPayload)
#endif // HMI_COMMON_EVENT_DEFS_H
