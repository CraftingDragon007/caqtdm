#ifndef HMI_COMMON_DEFS_H
#define HMI_COMMON_DEFS_H

#include <QDateTime> // For timestamp
#include <QByteArray>
#include <cstring>   // For memset

// Unique keys for shared memory and semaphore
#define SHARED_MEM_KEY "caQtDM_HmiSharedEventBus_SharedMem"
#define WRITE_LOCK_SEM_KEY "caQtDM_HmiSharedEventBus_WriteLock"

// Configuration parameters
#define MAX_PROCESS_SLOTS 16      // Maximum number of concurrent processes
#define EVENT_PAYLOAD_SIZE 256    // Fixed size for event data payload
#define EVENT_BUFFER_CAPACITY 100 // Max events in the ring buffer

// Define some event types for clarity
enum EventTypes {
    Unknown = 0,
    ApplicationStarted,
    KeyPress,
    MouseMove,
    MousePress
};

// Structure for a single event in the shared buffer
struct EventPayload {
    int eventType; // An integer ID for the event type (e.g., from EventTypes enum)
    int senderPid; // Process ID of the sender
    qint64 timestamp;
    char data[EVENT_PAYLOAD_SIZE]; // Flexible payload for event-specific data
    int dataSize; // Actual size of data in payload, up to EVENT_PAYLOAD_SIZE

    // Default constructor for initialization
    EventPayload() : eventType(0), senderPid(0), timestamp(0), dataSize(0) {
        std::memset(data, 0, EVENT_PAYLOAD_SIZE);
    }
};

// Header for the shared memory segment
struct SharedHeader {
    int currentWriteIndex;  // Next available slot for writing (0 to EVENT_BUFFER_CAPACITY - 1)
    int totalEventsWritten; // A monotonically increasing counter of all events ever written.
    // Used by processes to detect "new" events.

    // Each slot represents a potential active process.
    // pid = 0 means slot is free.
    // lastReadTotalEvents will track how many events *this specific slot* has consumed.
    struct ProcessSlot {
        int pid;                 // Process ID of the active process in this slot (0 if free)
        int lastReadTotalEvents; // totalEventsWritten that this process has last read
    } processSlots[MAX_PROCESS_SLOTS];

    // Default constructor for initialization
    SharedHeader() : currentWriteIndex(0), totalEventsWritten(0) {
        for (int i = 0; i < MAX_PROCESS_SLOTS; ++i) {
            processSlots[i].pid = 0;
            processSlots[i].lastReadTotalEvents = 0;
        }
    }
};

// Total size of shared memory needed:
// sizeof(SharedHeader) + EVENT_BUFFER_CAPACITY * sizeof(EventPayload)
#endif // HMI_COMMON_DEFS_H
