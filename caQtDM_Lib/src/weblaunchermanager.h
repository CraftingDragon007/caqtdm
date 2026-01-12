#ifndef WEBLAUNCHERMANAGER_H
#define WEBLAUNCHERMANAGER_H

#include <QObject>

class WebLauncherManager : public QObject
{
    Q_OBJECT
public:
    explicit WebLauncherManager(QObject *parent = nullptr);

signals:
};

#endif // WEBLAUNCHERMANAGER_H
