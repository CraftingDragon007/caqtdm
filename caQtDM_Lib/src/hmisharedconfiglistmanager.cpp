#include "hmisharedconfiglistmanager.h"
#include "causerid.h"

#include <QIODevice>

#define SHARED_MEMORY_LIST_KEY "caQtDM_HmiSharedConfigList_SharedMem_%1"
#define MAX_SHARED_MEMORY_SIZE 1024 * 1024 // 1 MB
#define PREFIX "HmiSharedConfigList"
#define MIN_SERIALIZED_ITEM_SIZE 32

HmiSharedConfigListManager::HmiSharedConfigListManager(QObject *parent)
    : QObject{parent},
    this_isInitialized(false),
    this_sharedMemory(QString(SHARED_MEMORY_LIST_KEY).arg(getUniqueUserId()))
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
            qCCritical(caHMILog) << "Failed to create or attach shared memory:" << this_sharedMemory.errorString();
            return false;
        }
        qCDebug(caHMILog) << "HMI Shared memory created and attached.";

        if (this_sharedMemory.lock()) {
            quint32 initialSize = 0;
            memcpy(this_sharedMemory.data(), &initialSize, sizeof(quint32));
            this_sharedMemory.unlock();
        } else {
            qCCritical(caHMILog) << "Failed to lock shared memory during initial write:" << this_sharedMemory.errorString();
            return false;
        }
    } else {
        qCDebug(caHMILog) << "Shared memory attached to existing segment.";
    }
    this_isInitialized = true;
    return true;
}

void HmiSharedConfigListManager::shutdown() {
    this_isInitialized = false;
    if (this_sharedMemory.isAttached()) {
        if (!this_sharedMemory.detach()) {
            qCWarning(caHMILog) << "Failed to detach from shared memory:" << this_sharedMemory.errorString();
        } else {
            qCDebug(caHMILog) << "Detached from shared memory.";
        }
    }
}

HmiSharedConfigListManager& HmiSharedConfigListManager::instance() {
    static HmiSharedConfigListManager manager;
    return manager;
}

QByteArray HmiSharedConfigListManager::readRawDataLocked() {
    QByteArray rawData;
    quint32 dataSize = 0;

    if (this_sharedMemory.constData() && static_cast<size_t>(this_sharedMemory.size()) >= sizeof(quint32)) {
        memcpy(&dataSize, this_sharedMemory.constData(), sizeof(quint32));
    } else {
        qCWarning(caHMILog) << "Shared memory is empty or too small to read data size.";
        return rawData;
    }

    if (dataSize > 0 && (sizeof(quint32) + dataSize) <= static_cast<size_t>(this_sharedMemory.size())) {
        const char* dataPtr = static_cast<const char*>(this_sharedMemory.constData()) + sizeof(quint32);
        rawData = QByteArray(dataPtr, static_cast<int>(dataSize)); // Copy actual data
    } else if (dataSize == 0) {
        qCDebug(caHMILog) << "Shared memory contains an empty list.";
    } else {
        qCWarning(caHMILog) << "Invalid data size detected in shared memory or shared memory too small. Data size:" << dataSize << "Shared memory size:" << this_sharedMemory.size();
    }
    return rawData;
}

bool HmiSharedConfigListManager::writeRawDataLocked(const QByteArray &serializedData) {
    quint32 dataSize = static_cast<quint32>(serializedData.size());
    quint32 totalRequiredSize = sizeof(quint32) + dataSize;

    if (totalRequiredSize > static_cast<quint32>(this_sharedMemory.size())) {
        qCCritical(caHMILog) << "New list is too large to fit in shared memory."
                    << "Required:" << totalRequiredSize << "Available:" << this_sharedMemory.size();
        return false;
    }

    char* memPtr = static_cast<char*>(this_sharedMemory.data());
    quint32 invalidSize = 0;
    memcpy(memPtr, &invalidSize, sizeof(quint32));
    memcpy(memPtr + sizeof(quint32), serializedData.constData(), dataSize);
    memcpy(memPtr, &dataSize, sizeof(quint32));
    qCDebug(caHMILog) << "List successfully written to shared memory. Size:" << dataSize << "bytes.";
    return true;
}

QList<QSharedPointer<caHMIConfigTransferItem>> HmiSharedConfigListManager::deserializeList(const QByteArray &rawData) {
    QList<QSharedPointer<caHMIConfigTransferItem>> list;
    if (rawData.isEmpty()) {
        return list;
    }

    QDataStream stream(rawData);
    quint32 count = 0;
    stream >> count;

    if (count > static_cast<quint32>(rawData.size()) / MIN_SERIALIZED_ITEM_SIZE) {
        qCWarning(caHMILog) << "Implausible item count" << count << "in shared memory list ("
                            << rawData.size() << "bytes), discarding list.";
        return list;
    }

    for (quint32 i = 0; i < count; ++i) {
        QSharedPointer<caHMIConfigTransferItem> config = QSharedPointer<caHMIConfigTransferItem>::create();
        stream >> *config.data();
        if (stream.status() != QDataStream::Ok) {
            qCWarning(caHMILog) << "Corrupted list data in shared memory (item" << i + 1 << "of"
                                << count << "), discarding list.";
            list.clear();
            return list;
        }
        list.append(config);
    }
    return list;
}

QByteArray HmiSharedConfigListManager::serializeList(const QList<QSharedPointer<caHMIConfigTransferItem>> &list) {
    QByteArray serializedData;
    QDataStream stream(&serializedData, QIODevice::WriteOnly);

    stream << static_cast<quint32>(list.size());

    foreach(QSharedPointer<caHMIConfigTransferItem> item, list) {
        stream << *item;
    }
    return serializedData;
}

QList<QSharedPointer<caHMIConfigTransferItem>> HmiSharedConfigListManager::readList() {
    QByteArray rawDataFromSharedMemory;

    if (this_sharedMemory.lock()) {
        rawDataFromSharedMemory = readRawDataLocked();
        this_sharedMemory.unlock();
    } else {
        qCCritical(caHMILog) << "Failed to lock shared memory for reading:" << this_sharedMemory.errorString();
        return QList<QSharedPointer<caHMIConfigTransferItem>>();
    }

    return deserializeList(rawDataFromSharedMemory);
}

bool HmiSharedConfigListManager::writeList(const QList<QSharedPointer<caHMIConfigTransferItem>> &newList) {
    QByteArray serializedData = serializeList(newList);

    bool ok = false;
    if (this_sharedMemory.lock()) {
        ok = writeRawDataLocked(serializedData);
        this_sharedMemory.unlock();
    } else {
        qCCritical(caHMILog) << "Failed to lock shared memory for writing:" << this_sharedMemory.errorString();
        return false;
    }

    if (ok) {
        emit dataChanged();
        // Note: This signal is only emitted within the current process.
    }
    return ok;
}

bool HmiSharedConfigListManager::updateList(const std::function<void(QList<QSharedPointer<caHMIConfigTransferItem>>&)> &mutator) {
    if (!this_sharedMemory.lock()) {
        qCCritical(caHMILog) << "Failed to lock shared memory for update:" << this_sharedMemory.errorString();
        return false;
    }

    QList<QSharedPointer<caHMIConfigTransferItem>> list = deserializeList(readRawDataLocked());
    mutator(list);
    bool ok = writeRawDataLocked(serializeList(list));

    this_sharedMemory.unlock();

    if (ok) {
        emit dataChanged();
        // Note: This signal is only emitted within the current process.
    }
    return ok;
}
