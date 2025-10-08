#include "hmisharedconfiglistmanager.h"
#include "causerid.h"

#include <QIODevice>

#define SHARED_MEMORY_LIST_KEY "caQtDM_HmiSharedConfigList_SharedMem_%1"
#define LIST_SEMAPHORE_KEY "caQtDM_HmiSharedConfigList_Semaphore_%1"
#define MAX_SHARED_MEMORY_SIZE 1024 * 1024 // 1 MB
#define PREFIX "HmiSharedConfigList"

HmiSharedConfigListManager::HmiSharedConfigListManager(QObject *parent)
    : QObject{parent},
    this_isInitialized(false),
    this_sharedMemory(QString(SHARED_MEMORY_LIST_KEY).arg(getUniqueUserId())),
    this_semaphore(QString(LIST_SEMAPHORE_KEY).arg(getUniqueUserId()), 1, QSystemSemaphore::Open)
{}

HmiSharedConfigListManager::~HmiSharedConfigListManager()
{
    shutdown();
}

bool HmiSharedConfigListManager::isInitialized() const {
    return this_isInitialized;
}

bool HmiSharedConfigListManager::setup() {
    if (!this_sharedMemory.attach()) {
        if (!this_sharedMemory.create(MAX_SHARED_MEMORY_SIZE)) {
            qCritical() << PREFIX << "Failed to create or attach shared memory:" << this_sharedMemory.errorString();
            return false;
        }
        qDebug() << PREFIX << "Shared memory created and attached.";

        if (!this_semaphore.acquire()) {
            qCritical() << PREFIX << "Failed to acquire semaphore during initial write:" << this_semaphore.errorString();
            return false;
        }
        if (this_sharedMemory.lock()) {
            quint32 initialSize = 0;
            memcpy(this_sharedMemory.data(), &initialSize, sizeof(quint32));
            this_sharedMemory.unlock();
        } else {
            qCritical() << PREFIX << "Failed to lock shared memory during initial write:" << this_sharedMemory.errorString();
        }
        this_semaphore.release();
    } else {
        qDebug() << PREFIX << "Shared memory attached to existing segment.";
    }
    this_isInitialized = true;
    return true;
}

void HmiSharedConfigListManager::shutdown() {
    if (this_sharedMemory.isAttached()) {
        if (!this_sharedMemory.detach()) {
            qWarning() << PREFIX << "Failed to detach from shared memory:" << this_sharedMemory.errorString();
        } else {
            qDebug() << PREFIX << "Detached from shared memory.";
        }
    }
}

HmiSharedConfigListManager& HmiSharedConfigListManager::instance() {
    static HmiSharedConfigListManager manager;
    return manager;
}

QList<QSharedPointer<caHMIConfigTransferItem>> HmiSharedConfigListManager::readList() {
    QList<QSharedPointer<caHMIConfigTransferItem>> list;
    QByteArray rawDataFromSharedMemory;

    if (!this_semaphore.acquire()) {
        qCritical() << PREFIX << "Failed to acquire semaphore for reading:" << this_semaphore.errorString();
        return list;
    }

    if (this_sharedMemory.lock()) {
        quint32 dataSize = 0;
        if (this_sharedMemory.constData() && static_cast<size_t>(this_sharedMemory.size()) >= sizeof(quint32)) {
            memcpy(&dataSize, this_sharedMemory.constData(), sizeof(quint32));
        } else {
            qWarning() << PREFIX << "Shared memory is empty or too small to read data size.";
            this_sharedMemory.unlock();
            this_semaphore.release();
            return list;
        }

        if (dataSize > 0 && (sizeof(quint32) + dataSize) <= static_cast<size_t>(this_sharedMemory.size())) {
            const char* dataPtr = static_cast<const char*>(this_sharedMemory.constData()) + sizeof(quint32);
            rawDataFromSharedMemory = QByteArray(dataPtr, static_cast<int>(dataSize)); // Copy actual data
        } else if (dataSize == 0) {
            qDebug() << PREFIX << "Shared memory contains an empty list.";
        } else {
            qWarning() << PREFIX << "Invalid data size detected in shared memory or shared memory too small. Data size:" << dataSize << "Shared memory size:" << this_sharedMemory.size();
        }
        this_sharedMemory.unlock();
    } else {
        qCritical() << PREFIX << "Failed to lock shared memory for reading:" << this_sharedMemory.errorString();
    }
    this_semaphore.release();

    if (!rawDataFromSharedMemory.isEmpty()) {
        QDataStream stream(&rawDataFromSharedMemory, QIODevice::ReadOnly);
        quint32 count = 0;
        stream >> count; // Read the count of items

        for (quint32 i = 0; i < count; ++i) {
            QSharedPointer<caHMIConfigTransferItem> config = QSharedPointer<caHMIConfigTransferItem>::create();
            stream >> *config.data(); // Deserialize directly into the object
            list.append(config);
        }
    }
    return list;
}

bool HmiSharedConfigListManager::writeList(const QList<QSharedPointer<caHMIConfigTransferItem>> &newList) {
    QByteArray serializedData;
    QDataStream stream(&serializedData, QIODevice::WriteOnly);

    // Write the count of items first
    stream << static_cast<quint32>(newList.size());

    // Then write each item directly
    foreach(QSharedPointer<caHMIConfigTransferItem> item, newList) {
        stream << *item; // Dereference the QSharedPointer and serialize the object
    }

    quint32 dataSize = static_cast<quint32>(serializedData.size());
    quint32 totalRequiredSize = sizeof(quint32) + dataSize;

    if (totalRequiredSize > MAX_SHARED_MEMORY_SIZE) {
        qCritical() << PREFIX << "New list is too large to fit in shared memory."
                    << "Required:" << totalRequiredSize << "Available:" << MAX_SHARED_MEMORY_SIZE;
        return false;
    }

    if (!this_semaphore.acquire()) {
        qCritical() << PREFIX << "Failed to acquire semaphore for writing:" << this_semaphore.errorString();
        return false;
    }

    if (this_sharedMemory.lock()) {
        memcpy(this_sharedMemory.data(), &dataSize, sizeof(quint32));
        memcpy(static_cast<char*>(this_sharedMemory.data()) + sizeof(quint32), serializedData.constData(), dataSize);
        this_sharedMemory.unlock();
        qDebug() << PREFIX << "List successfully written to shared memory. Size:" << dataSize << "bytes.";
        emit dataChanged();
        // Note: This signal is only emitted within the current process.
    } else {
        qCritical() << PREFIX << "Failed to lock shared memory for writing:" << this_sharedMemory.errorString();
        this_semaphore.release();
        return false;
    }
    this_semaphore.release();
    return true;
}
