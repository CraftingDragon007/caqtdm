#ifndef CAHMICONFIG_H
#define CAHMICONFIG_H

#include "hmiapplicationeventfilter.h"

#include <QWidget>

#include <quuid.h>

#include <qtcontrols_global.h>

class QTCON_EXPORT caHMIConfig : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QKeySequence shortcut READ shortcutAsSequence WRITE setShortcutFromSequence DESIGNABLE true)
    Q_PROPERTY(QString channel READ channel WRITE setChannel DESIGNABLE true)
    Q_PROPERTY(QString valueOrCalc READ value WRITE setValue DESIGNABLE true)
    Q_PROPERTY(calcType calculationType READ calculationType WRITE setCalculationType DESIGNABLE true)
    Q_PROPERTY(capType captureType READ captureType WRITE setCaptureType DESIGNABLE true)
public:
    enum calcType {SetValue, Calc};
    enum capType {Mouse, KeyboardValue, KeyboardSet};
    Q_ENUM(calcType);
    Q_ENUM(capType);
    explicit caHMIConfig(QWidget *parent = nullptr);

    void setShortcutFromSequence(const QKeySequence &key);
    QKeySequence shortcutAsSequence() const;
    QKeyCombination shortcut() const;

    void setChannel(const QString &channel);
    QString channel() const;

    void setValue(const QString &value);
    QString value() const;

    void setCalculationType(const calcType &calculationType);
    calcType calculationType() const;

    void setCaptureType(const capType &captureType);
    capType captureType() const;

    QString uuid() const;

private:
    QKeyCombination thisKey;
    QString thisChannel;
    QVariant thisValue = 1;
    calcType thisCalculationType = calcType::SetValue;
    capType thisCaptureType = capType::KeyboardSet;
    QUuid thisUUID;
    void processEvent(QEvent *event);

public slots:
    void handleKeyPressed(QObject *target, QKeyEvent *event);
    void handleMousePressed(QObject *target, QMouseEvent *event);

protected:

signals:
    void HMIConfigInputReceived(int *data);

};

#endif // CAHMICONFIG_H
