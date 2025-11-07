#ifndef HMISHAREDCONFIGLISTMANAGER_H
#define HMISHAREDCONFIGLISTMANAGER_H

#include "cahmiconfigtransferitem.h"
#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QDataStream>
#include <QByteArray>
#include <QList>
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

private:
    explicit HmiSharedConfigListManager(QObject *parent = nullptr);
    ~HmiSharedConfigListManager();
    bool this_isInitialized;
    QSharedMemory this_sharedMemory;
    QSystemSemaphore this_semaphore;

    HmiSharedConfigListManager(const HmiSharedConfigListManager&) = delete;
    HmiSharedConfigListManager& operator=(const HmiSharedConfigListManager&) = delete;

signals:
    void dataChanged();
};

#endif // HMISHAREDCONFIGLISTMANAGER_H
