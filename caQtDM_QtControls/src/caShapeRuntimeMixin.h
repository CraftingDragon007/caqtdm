#ifndef CASHAPERUNTIMEMIXIN_H
#define CASHAPERUNTIMEMIXIN_H

#include "caWidgetUpdateInterface.h"
#include "alarmdefs.h"

#include <QColor>
#include <QSize>
#include <QWidget>

class caShapeRuntime : public caWidgetUpdateInterface
{
public:
    virtual int shapeObjectType() const = 0;
    virtual void setActualSize(QSize size) { Q_UNUSED(size); }
};

template <typename Widget>
class caShapeRuntimeMixin : public caShapeRuntime
{
public:
    bool needsAggregateAlarm() const override
    {
        return static_cast<const Widget *>(this)->getColorMode() == Widget::Alarm;
    }
    bool needsVisibilityUpdate() const override { return true; }

    caWidgetUpdateResult applyChannelUpdate(const QString &units, const QString &value,
                                            const knobData &data, short aggregateAlarm) override
    {
        Q_UNUSED(units);
        Q_UNUSED(value);
        Widget *w = static_cast<Widget *>(this);
        w->logShapeUpdate(data);
        if (!data.edata.connected) {
            w->setAlarmColors(NOTCONNECTED);
            w->setProperty("Connect", false);
        } else if (w->getColorMode() == Widget::Static) {
            if (!w->property("Connect").toBool()) {
                w->setForeground(w->property("FColor").template value<QColor>());
                w->setLineColor(w->property("LColor").template value<QColor>());
                w->setProperty("Connect", true);
            }
        } else if (w->getColorMode() == Widget::Alarm) {
            w->setAlarmColors(aggregateAlarm);
        }
        return {};
    }
};

#endif // CASHAPERUNTIMEMIXIN_H
