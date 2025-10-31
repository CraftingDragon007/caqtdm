#ifndef SHAREDVNCLISTMANAGER_H
#define SHAREDVNCLISTMANAGER_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QList>
#include "caQtDM_Lib_global.h"
#include "vncpanelinstance.h"

class CAQTDM_LIBSHARED_EXPORT SharedVNCListManager : public QObject
{
    Q_OBJECT
public:
    static SharedVNCListManager& instance();

    bool setup();
    void shutdown();
    bool isInitialized() const;

    QList<VNCPanelInstance> readList();
    bool writeList(const QList<VNCPanelInstance> &newList);

private:
    explicit SharedVNCListManager(QObject *parent = nullptr);
    ~SharedVNCListManager();

    bool m_isInitialized;
    QSharedMemory m_sharedMemory;
    QSystemSemaphore m_semaphore;

    SharedVNCListManager(const SharedVNCListManager&) = delete;
    SharedVNCListManager& operator=(const SharedVNCListManager&) = delete;

signals:
    void dataChanged();
};

#endif // SHAREDVNCLISTMANAGER_H
