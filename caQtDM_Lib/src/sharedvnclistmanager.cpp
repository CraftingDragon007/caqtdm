#include "sharedvnclistmanager.h"
#include "qdebug.h"
#include <causerid.h>

#include <QIODevice>

#define SHARED_MEMORY_LIST_KEY "caQtDM_SharedVNCList_SharedMem_%1"
#define LIST_SEMAPHORE_KEY "caQtDM_SharedVNCList_SharedMem_%1"
#define MAX_SHARED_MEMORY_SIZE 1024 * 1024 // 1 MB
#define PREFIX "SharedVNCList"

SharedVNCListManager::SharedVNCListManager(QObject *parent)
    : QObject{parent},
    m_isInitialized(false),
    m_sharedMemory(QString(SHARED_MEMORY_LIST_KEY).arg(getUniqueUserId())),
    m_semaphore(QString(LIST_SEMAPHORE_KEY).arg(getUniqueUserId()), 1, QSystemSemaphore::Open)
{}

SharedVNCListManager::~SharedVNCListManager()
{
    shutdown();
}

bool SharedVNCListManager::isInitialized() const {
    return m_isInitialized;
}

bool SharedVNCListManager::setup() {
    if (!m_sharedMemory.attach()) {
        if (!m_sharedMemory.create(MAX_SHARED_MEMORY_SIZE)) {
            qCritical() << PREFIX << "Failed to create or attach shared memory:" << m_sharedMemory.errorString();
            return false;
        }
        qDebug() << PREFIX << "Shared memory created and attached.";

        if (!m_semaphore.acquire()) {
            qCritical() << PREFIX << "Failed to acquire semaphore during initial write:" << m_semaphore.errorString();
            return false;
        }
        if (m_sharedMemory.lock()) {
            quint32 initialSize = 0;
            memcpy(m_sharedMemory.data(), &initialSize, sizeof(quint32));
            m_sharedMemory.unlock();
        } else {
            qCritical() << PREFIX << "Failed to lock shared memory during initial write:" << m_sharedMemory.errorString();
        }
        m_semaphore.release();
    } else {
        qDebug() << PREFIX << "Shared memory attached to existing segment.";
    }
    m_isInitialized = true;
    return true;
}

void SharedVNCListManager::shutdown() {
    if (m_sharedMemory.isAttached()) {
        if (!m_sharedMemory.detach()) {
            qWarning() << PREFIX << "Failed to detach from shared memory:" << m_sharedMemory.errorString();
        } else {
            qDebug() << PREFIX << "Detached from shared memory.";
        }
    }
}

SharedVNCListManager& SharedVNCListManager::instance() {
    static SharedVNCListManager manager;
    return manager;
}

QList<VNCPanelInstance> SharedVNCListManager::readList() {
    QList<VNCPanelInstance> list;
    QByteArray rawDataFromSharedMemory;

    if (!m_semaphore.acquire()) {
        qCritical() << PREFIX << "Failed to acquire semaphore for reading:" << m_semaphore.errorString();
        return list;
    }

    if (m_sharedMemory.lock()) {
        quint32 dataSize = 0;
        if (m_sharedMemory.constData() && static_cast<size_t>(m_sharedMemory.size()) >= sizeof(quint32)) {
            memcpy(&dataSize, m_sharedMemory.constData(), sizeof(quint32));
        } else {
            qWarning() << PREFIX << "Shared memory is empty or too small to read data size.";
            m_sharedMemory.unlock();
            m_semaphore.release();
            return list;
        }

        if (dataSize > 0 && (sizeof(quint32) + dataSize) <= static_cast<size_t>(m_sharedMemory.size())) {
            const char* dataPtr = static_cast<const char*>(m_sharedMemory.constData()) + sizeof(quint32);
            rawDataFromSharedMemory = QByteArray(dataPtr, static_cast<int>(dataSize)); // Copy actual data
        } else if (dataSize == 0) {
            qDebug() << PREFIX << "Shared memory contains an empty list.";
        } else {
            qWarning() << PREFIX << "Invalid data size detected in shared memory or shared memory too small. Data size:" << dataSize << "Shared memory size:" << m_sharedMemory.size();
        }
        m_sharedMemory.unlock();
    } else {
        qCritical() << PREFIX << "Failed to lock shared memory for reading:" << m_sharedMemory.errorString();
    }
    m_semaphore.release();

    if (!rawDataFromSharedMemory.isEmpty()) {
        QDataStream stream(&rawDataFromSharedMemory, QIODevice::ReadOnly);
        quint32 count = 0;
        stream >> count;

        for (quint32 i = 0; i < count; ++i) {
            VNCPanelInstance item;
            stream >> item;
            list.append(item);
        }
    }
    return list;
}

bool SharedVNCListManager::writeList(const QList<VNCPanelInstance> &newList) {
    QByteArray serializedData;
    QDataStream stream(&serializedData, QIODevice::WriteOnly);

    stream << static_cast<quint32>(newList.size());

    foreach(VNCPanelInstance item, newList) {
        stream << item;
    }

    quint32 dataSize = static_cast<quint32>(serializedData.size());
    quint32 totalRequiredSize = sizeof(quint32) + dataSize;

    if (totalRequiredSize > MAX_SHARED_MEMORY_SIZE) {
        qCritical() << PREFIX << "New list is too large to fit in shared memory."
                    << "Required:" << totalRequiredSize << "Available:" << MAX_SHARED_MEMORY_SIZE;
        return false;
    }

    if (!m_semaphore.acquire()) {
        qCritical() << PREFIX << "Failed to acquire semaphore for writing:" << m_semaphore.errorString();
        return false;
    }

    if (m_sharedMemory.lock()) {
        memcpy(m_sharedMemory.data(), &dataSize, sizeof(quint32));
        memcpy(static_cast<char*>(m_sharedMemory.data()) + sizeof(quint32), serializedData.constData(), dataSize);
        m_sharedMemory.unlock();
        //qDebug() << PREFIX << "List successfully written to shared memory. Size:" << dataSize << "bytes.";
        emit dataChanged();
        // Note: This signal is only emitted within the current process.
    } else {
        qCritical() << PREFIX << "Failed to lock shared memory for writing:" << m_sharedMemory.errorString();
        m_semaphore.release();
        return false;
    }
    m_semaphore.release();
    return true;
}
