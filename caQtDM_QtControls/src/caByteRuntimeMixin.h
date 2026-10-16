#ifndef CABYTERUNTIMEMIXIN_H
#define CABYTERUNTIMEMIXIN_H

#include "caWidgetUpdateInterface.h"
#include "alarmdefs.h"

#include <QWidget>

class caByteRuntime : public caWidgetUpdateInterface
{
public:
    virtual int byteObjectType() const = 0;
    virtual QString bytePV() const = 0;
    virtual void setBytePV(const QString &pv) = 0;
    virtual bool byteUsesAlarmColorMode() const = 0;
};

template <typename Widget>
class caByteRuntimeMixin : public caByteRuntime
{
public:
    bool needsAggregateAlarm() const override { return false; }
    bool needsVisibilityUpdate() const override { return false; }
    QString bytePV() const override { return widget()->getPV(); }
    void setBytePV(const QString &pv) override { widget()->setPV(pv); }
    bool byteUsesAlarmColorMode() const override
    {
        return widget()->getColorMode() == Widget::Alarm;
    }

    caWidgetUpdateResult applyChannelUpdate(const QString &units, const QString &value,
                                            const knobData &data, short aggregateAlarm) override
    {
        Q_UNUSED(units);
        Q_UNUSED(value);
        Q_UNUSED(aggregateAlarm);
        Widget *w = widget();
        if (!data.edata.connected) {
            w->setAlarmColors(NOTCONNECTED);
            w->setProperty("Connect", false);
        } else if (w->getColorMode() == Widget::Static) {
            if (!w->property("Connect").toBool())
                w->setProperty("Connect", true);
            w->setValue(data.edata.ivalue);
        } else if (w->getColorMode() == Widget::Alarm) {
            w->setValue(data.edata.ivalue);
            w->setAlarmColors(data.edata.severity);
        }
        return {};
    }

private:
    Widget *widget() { return static_cast<Widget *>(this); }
    const Widget *widget() const { return static_cast<const Widget *>(this); }
};

#endif // CABYTERUNTIMEMIXIN_H
