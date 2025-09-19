#ifndef CAHMICONFIG_H
#define CAHMICONFIG_H

#include <QWidget>

#include <quuid.h>

#include <qtcontrols_global.h>

class QTCON_EXPORT caHMIConfig : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString channel READ channel WRITE setChannel DESIGNABLE true)
    Q_PROPERTY(QString channelB READ channelB WRITE setChannelB DESIGNABLE true)
    Q_PROPERTY(QString channelC READ channelC WRITE setChannelC DESIGNABLE true)
    Q_PROPERTY(QString channelD READ channelD WRITE setChannelD DESIGNABLE true)
    Q_PROPERTY(QKeySequence shortcut READ shortcutAsSequence WRITE setShortcutFromSequence DESIGNABLE true)
    Q_PROPERTY(QString valueOrCalc READ value WRITE setValue DESIGNABLE true)
    Q_PROPERTY(calcType calculationType READ calculationType WRITE setCalculationType DESIGNABLE true)
    Q_PROPERTY(capType captureType READ captureType WRITE setCaptureType DESIGNABLE true)
    Q_PROPERTY(QSize mouseSignalRectSize READ mouseRectSize WRITE setMouseRectSize DESIGNABLE true)
public:
    enum calcType {SetValue, Calc};
    enum capType {KeyboardValue, KeyboardSet, MouseMove, MousePress};
    Q_ENUM(calcType);
    Q_ENUM(capType);
    explicit caHMIConfig(QWidget *parent = nullptr);

    void setShortcutFromSequence(const QKeySequence &key);
    QKeySequence shortcutAsSequence() const;
    QKeyCombination shortcut() const;

    void setChannel(const QString &channel);
    QString channel() const;

    void setChannelB (const QString &channel);
    QString channelB() const;

    void setChannelC (const QString &channel);
    QString channelC() const;

    void setChannelD (const QString &channel);
    QString channelD() const;

    void setValue(const QString &value);
    QString value() const;

    void setCalculationType(const calcType &calculationType);
    calcType calculationType() const;

    void setCaptureType(const capType &captureType);
    capType captureType() const;

    void setMouseRectSize(const QSize &rectSize);
    QSize mouseRectSize() const;

    QString uuid() const;

    static bool isDesignerMode();

private:
    QKeyCombination thisKey;
    QString thisChannelA;
    QString thisChannelB;
    QString thisChannelC;
    QString thisChannelD;
    QVariant thisValue = 1;
    calcType thisCalculationType = calcType::SetValue;
    capType thisCaptureType = capType::KeyboardSet;
    QSize thisMouseRectSize = QSize(10, 10);
    QUuid thisUUID;

public slots:

protected:
    void paintEvent(QPaintEvent *ev) override;

signals:
    void caHMIConfigInputReceived(int data);
    void caHMIConfigKeyPressReceived(QKeyCombination data);
    void caHMIConfigMouseX(int x);
    void caHMIConfigMouseY(int y);
    void caHMIConfigMouse(QRect rect);
    void caHMIConfigMouse(QPoint point);
    void caHMIConfigValueSet(QVariant value);

};

#endif // CAHMICONFIG_H
