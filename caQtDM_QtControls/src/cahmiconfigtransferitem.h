#ifndef CAHMICONFIGTRANSFERITEM_H
#define CAHMICONFIGTRANSFERITEM_H

#include "cahmiconfig.h"
#include "qvariant.h"
#include <QObject>
#include <qtcontrols_global.h>

class QTCON_EXPORT caHMIConfigTransferItem : public QObject
{
    Q_OBJECT
public:
    explicit caHMIConfigTransferItem(QObject *parent = nullptr);

    void setEnabled(const bool &enabled) { this->thisEnabled = enabled; }
    bool enabled() const { return this->thisEnabled; }

    void setObjectName(const QString &name) { this->thisObjectName = name; }
    QString objectName() const { return this->thisObjectName; }

    void setFileName(const QString &name) { this->thisFileName = name; }
    QString fileName() const { return this->thisFileName; }

    void setOutputA(const QString &outputA) { this->thisOutputA = outputA; }
    QString outputA() const { return this->thisOutputA; }

    void setOutputB(const QString &outputB) { this->thisOutputB = outputB; }
    QString outputB() const { return this->thisOutputB; }

    void setChannel(const QString &channel) { this->thisChannelA = channel; }
    QString channel() const { return this->thisChannelA; }

    void setChannelB(const QString &channel) { this->thisChannelB = channel; }
    QString channelB() const { return this->thisChannelB; }

    void setChannelC(const QString &channel) { this->thisChannelC = channel; }
    QString channelC() const { return this->thisChannelC; }

    void setChannelD(const QString &channel) { this->thisChannelD = channel; }
    QString channelD() const { return this->thisChannelD; }

    void setShortcut(const QKeySequence &shortcut) { this->thisShortcut = shortcut; }
    QKeySequence shortcut() const { return this->thisShortcut; }

    void setValue(const QVariant &value) { this->thisValue = value; }
    QVariant value() const { return this->thisValue; }

    void setCalculationType(const caHMIConfig::calcType calculationType) { this->thisCalculationType = calculationType; }
    caHMIConfig::calcType calculationType() const { return this->thisCalculationType; }

    void setCaptureType(const caHMIConfig::capType captureType) { this->thisCaptureType = captureType; }
    caHMIConfig::capType captureType() const { return this->thisCaptureType; }

    void setCaptureRange(const caHMIConfig::capRange captureRange) { this->thisCaptureRange = captureRange; }
    caHMIConfig::capRange captureRange() const { return this->thisCaptureRange; }

    void setPID(const int pid) { this->thisPID = pid; }
    int pid() const { return this->thisPID; }

    void setUUID(const QString uuid) { this->thisUUID = uuid; }
    QString uuid() const { return this->thisUUID; }

    void setWidgetCallback(caHMIConfig* widget) { this->thisWidgetCallback = widget; }
    caHMIConfig* widgetCallback() const { return this->thisWidgetCallback; }

    void setParentWindowCallback(QWidget* parent) { this->parent = parent; }
    QWidget* parentWindowCallback() const { return this->parent; }

    friend QDataStream& operator<<(QDataStream& out, const caHMIConfigTransferItem& config) {
        out << config.uuid() << config.pid() << config.enabled() << config.objectName() << config.fileName()
            << config.outputA() << config.outputB() << config.channel() << config.channelB()
            << config.channelC() << config.channelD() << config.shortcut() << config.value()
            << config.calculationType() << config.captureType() << config.captureRange();
        return out;
    }

    friend QDataStream& operator>>(QDataStream& in, caHMIConfigTransferItem& config) {
        QString uuid;
        int pid;
        bool enabled;
        QString objectName;
        QString fileName;
        QString outputA;
        QString outputB;
        QString channel;
        QString channelB;
        QString channelC;
        QString channelD;
        QKeySequence shortcut;
        QVariant value;
        caHMIConfig::calcType calculationType;
        caHMIConfig::capType captureType;
        caHMIConfig::capRange captureRange;

        in >> uuid >> pid >> enabled >> objectName >> fileName
            >> outputA >> outputB >> channel >> channelB
            >> channelC >> channelD >> shortcut >> value
            >> calculationType >> captureType >> captureRange;

        config.setUUID(uuid);
        config.setPID(pid);
        config.setEnabled(enabled);
        config.setObjectName(objectName);
        config.setFileName(fileName);
        config.setOutputA(outputA);
        config.setOutputB(outputB);
        config.setChannel(channel);
        config.setChannelB(channelB);
        config.setChannelC(channelC);
        config.setChannelD(channelD);
        config.setShortcut(shortcut);
        config.setValue(value);
        config.setCalculationType(calculationType);
        config.setCaptureType(captureType);
        config.setCaptureRange(captureRange);

        return in;
    }

    virtual QSharedPointer<caHMIConfigTransferItem> clone() const;


private:
    bool thisEnabled;
    QString thisObjectName;
    QString thisFileName;
    QString thisOutputA;
    QString thisOutputB;
    QString thisChannelA;
    QString thisChannelB;
    QString thisChannelC;
    QString thisChannelD;
    QKeySequence thisShortcut;
    QVariant thisValue;
    caHMIConfig::calcType thisCalculationType;
    caHMIConfig::capType thisCaptureType;
    caHMIConfig::capRange thisCaptureRange;
    int thisPID;
    QString thisUUID;
    caHMIConfig* thisWidgetCallback;
    QWidget* parent;

signals:
};

#endif // CAHMICONFIGTRANSFERITEM_H
