#ifndef CAHMICONFIG_H
#define CAHMICONFIG_H

#include "hmiapplicationeventfilter.h"

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
    QKeyCombination thisKey;
    QKeyCombination* previousInput;
    QString thisChannel;
    HMIApplicationEventFilter* globalEventFilter;
    bool processEvent(QEvent *event);

private slots:
    void handleKeyPressed(QObject *target, QKeyEvent *event);
    void handleMousePressed(QObject *target, QMouseEvent *event);

protected:
    /*void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;*/

signals:
    void HMIConfigInputReceived(int *data);

};

#endif // CAHMICONFIG_H
