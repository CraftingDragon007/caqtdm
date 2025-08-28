#ifndef CAHMICONFIG_H
#define CAHMICONFIG_H

#include <QWidget>

#include <qtcontrols_global.h>

class QTCON_EXPORT caHMIConfig : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut DESIGNABLE true)
    Q_PROPERTY(QString channel READ channel WRITE setChannel DESIGNABLE true)

public:
    explicit caHMIConfig(QWidget *parent = nullptr);

    void setShortcut(const QKeySequence &key);
    QKeySequence shortcut() const;

    void setChannel(const QString &channel);
    QString channel() const;

private:
    QKeySequence thisKey;
    QString thisChannel;

signals:

};

#endif // CAHMICONFIG_H
