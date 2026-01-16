#ifndef WEBLAUNCHERMANAGER_H
#define WEBLAUNCHERMANAGER_H

#include <QObject>
#include <QJsonValue>
#include <QJsonArray>
#include "caQtDM_Lib_global.h"

class CAQTDM_LIBSHARED_EXPORT WebLauncherManager : public QObject
{
    Q_OBJECT
public:
    static WebLauncherManager& instance();

    bool setup(const QString fileName);
    bool isInitialized() const;
    QJsonValue getExpandedLauncherJson() const;

private:
    explicit WebLauncherManager(QObject *parent = nullptr);
    ~WebLauncherManager();

    bool m_isInitialized;

    QJsonValue m_expandedLauncherJson;

    QSet<QString> m_visitedFiles;
    QJsonValue loadAndExpand(QString fileName);
    QJsonValue expandObject(QJsonObject obj);
    QJsonArray expandArray(const QJsonArray &arr);

    WebLauncherManager(const WebLauncherManager&) = delete;
    WebLauncherManager& operator=(const WebLauncherManager&) = delete;

signals:
};

#endif // WEBLAUNCHERMANAGER_H
