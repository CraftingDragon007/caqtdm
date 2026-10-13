#ifndef CALABELUPDATEMIXIN_H
#define CALABELUPDATEMIXIN_H

#include "caWidgetUpdateInterface.h"
#include "alarmdefs.h"

#include <QColor>
#include <QWidget>

class caLabelUpdateMixin : public caWidgetUpdateInterface
{
public:
    enum ColorBehavior { StaticColors, AlarmColors, OtherColors };

    virtual int labelObjectType() const = 0;
    virtual QString labelText() const = 0;
    virtual void setLabelText(const QString &text) = 0;

    bool needsAggregateAlarm() const override { return colorBehavior() == AlarmColors; }
    bool needsVisibilityUpdate() const override { return true; }

    void applyChannelUpdate(const QString &units, const QString &value,
                            const knobData &data, short aggregateAlarm) override
    {
        Q_UNUSED(units);
        Q_UNUSED(value);

        QWidget *widget = labelWidget();
        if (!data.edata.connected) {
            setLabelAlarmColors(NOTCONNECTED);
            widget->setProperty("Connect", false);
            return;
        }

        if (colorBehavior() == StaticColors && !widget->property("Connect").toBool()) {
            restoreLabelColors(widget->property("FColor").value<QColor>(),
                               widget->property("BColor").value<QColor>());
            widget->setProperty("Connect", true);
        } else if (colorBehavior() == AlarmColors) {
            setLabelAlarmColors(aggregateAlarm);
        }
    }

protected:
    virtual QWidget *labelWidget() = 0;
    virtual ColorBehavior colorBehavior() const = 0;
    virtual void restoreLabelColors(const QColor &foreground, const QColor &background) = 0;
    virtual void setLabelAlarmColors(short status) = 0;
};

#endif // CALABELUPDATEMIXIN_H
