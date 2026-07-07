#ifndef HMISHAREDCONFIGLISTMANAGER_H
#define HMISHAREDCONFIGLISTMANAGER_H

#include "cahmiconfigtransferitem.h"
#include <QObject>
#include <QSharedMemory>
#include <QDataStream>
#include <QByteArray>
#include <QList>
#include <functional>
#include "caQtDM_Lib_global.h"

class CAQTDM_LIBSHARED_EXPORT HmiSharedConfigListManager : public QObject
{
    Q_OBJECT
public:
    static HmiSharedConfigListManager& instance();

    bool setup();
    void shutdown();
    bool isInitialized() const;

    QList<QSharedPointer<caHMIConfigTransferItem>> readList();
    bool writeList(const QList<QSharedPointer<caHMIConfigTransferItem>> &newList);
    bool updateList(const std::function<void(QList<QSharedPointer<caHMIConfigTransferItem>>&)> &mutator);

private:
    explicit HmiSharedConfigListManager(QObject *parent = nullptr);
    ~HmiSharedConfigListManager();
    bool this_isInitialized;
    QSharedMemory this_sharedMemory;

    QByteArray readRawDataLocked();
    bool writeRawDataLocked(const QByteArray &serializedData);
    QList<QSharedPointer<caHMIConfigTransferItem>> deserializeList(const QByteArray &rawData);
    QByteArray serializeList(const QList<QSharedPointer<caHMIConfigTransferItem>> &list);

    HmiSharedConfigListManager(const HmiSharedConfigListManager&) = delete;
    HmiSharedConfigListManager& operator=(const HmiSharedConfigListManager&) = delete;

signals:
    void dataChanged();
};

#endif // HMISHAREDCONFIGLISTMANAGER_H
