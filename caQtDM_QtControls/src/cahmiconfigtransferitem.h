#ifndef CAHMICONFIGTRANSFERITEM_H
#define CAHMICONFIGTRANSFERITEM_H

#include "cahmiconfig.h"
#include "qvariant.h"
#include <QObject>

class caHMIConfigTransferItem : public QObject
{
    Q_OBJECT
public:
    explicit caHMIConfigTransferItem(QObject *parent = nullptr);

    void setChannel(const QString &channel) { this->thisChannel = channel; }
    QString channel() const { return this->thisChannel; }

    void setShortcut(const QKeyCombination &shortcut) { this->thisShortcut = shortcut; }
    QKeyCombination shortcut() const { return this->thisShortcut; }

    void setValue(const QVariant &value) { this->thisValue = value; }
    QVariant value() const { return this->thisValue; }

    void setCalculationType(const caHMIConfig::calcType calculationType) { this->thisCalculationType = calculationType; }
    caHMIConfig::calcType calculationType() const { return this->thisCalculationType; }

    void setCaptureType(const caHMIConfig::capType captureType) { this->thisCaptureType = captureType; }
    caHMIConfig::capType captureType() const { return this->thisCaptureType; }

    void setPID(const int pid) { this->thisPID = pid; }
    int pid() const { return this->thisPID; }

    void setUUID(const QString uuid) { this->thisUUID = uuid; }
    QString uuid() const { return this->thisUUID; }

    void setWidgetCallback(caHMIConfig* widget) { this->thisWidgetCallback = widget; }
    caHMIConfig* widgetCallback() const { return this->thisWidgetCallback; }

private:
    QString thisChannel;
    QKeyCombination thisShortcut;
    QVariant thisValue;
    caHMIConfig::calcType thisCalculationType;
    caHMIConfig::capType thisCaptureType;
    int thisPID;
    QString thisUUID;
    caHMIConfig* thisWidgetCallback;

signals:
};

#endif // CAHMICONFIGTRANSFERITEM_H
