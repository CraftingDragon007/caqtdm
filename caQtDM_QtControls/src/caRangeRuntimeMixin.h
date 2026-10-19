#ifndef CARANGERUNTIMEMIXIN_H
#define CARANGERUNTIMEMIXIN_H

#include "caWidgetUpdateInterface.h"
#include "alarmdefs.h"

#include <QWidget>

struct caRangeDialogState
{
    bool separateLimitModes;
    bool initializeLimitsMode;
    bool userLimits;
    bool userHighLimit;
    bool userLowLimit;
    bool initializePrecisionMode;
    bool userPrecision;
    bool initializePrecisionValue;
    double minValue;
    double maxValue;
    int precision;
};

struct caRangeDialogApply
{
    bool highChannel;
    bool highUser;
    bool lowChannel;
    bool lowUser;
    int limitsMode;
    int precisionMode;
    double minValue;
    double maxValue;
    double channelLowerLimit;
    double channelUpperLimit;
    bool channelLimitsAvailable;
    int channelPrecision;
    int precision;
};

class caRangeRuntime : public caWidgetUpdateInterface
{
public:
    virtual int rangeObjectType() const = 0;
    virtual QString rangePV() const = 0;
    virtual void setRangePV(const QString &pv) = 0;
    virtual void prepareRange() = 0;
    virtual bool rangeIsElevated() = 0;
    virtual void raiseRange() = 0;
    virtual bool rangeUsesAlarmColorMode() const = 0;
    virtual caRangeDialogState rangeDialogState(int channelPrecision) const = 0;
    virtual void applyRangeDialog(const caRangeDialogApply &state) = 0;
    virtual void restoreRangeDialogValue(double value) = 0;
};

template <typename Widget>
class caRangeRuntimeMixin : public caRangeRuntime
{
public:
    bool needsAggregateAlarm() const override { return false; }
    bool needsVisibilityUpdate() const override { return false; }
    QString rangePV() const override { return widget()->getPV(); }
    void setRangePV(const QString &pv) override { widget()->setPV(pv); }
    bool rangeUsesAlarmColorMode() const override
    {
        return widget()->getColorMode() == Widget::Alarm_Default ||
               widget()->getColorMode() == Widget::Alarm_Static;
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
            return {};
        }

        const bool channelLimitsEnabled = applyRangeValue(data);
        if (w->getColorMode() == Widget::Alarm_Default ||
            w->getColorMode() == Widget::Alarm_Static) {
            if (channelLimitsEnabled) w->setAlarmColors(data.edata.severity);
            else w->setUserAlarmColors(data.edata.rvalue);
        } else if (!w->property("Connect").toBool()) {
            w->setNormalColors();
            w->setProperty("Connect", true);
        }

        if (w->getPrecisionMode() != Widget::User && data.edata.initialize)
            w->setPrecision(data.edata.precision);
        return {};
    }

protected:
    virtual bool applyRangeValue(const knobData &data) = 0;

private:
    Widget *widget() { return static_cast<Widget *>(this); }
    const Widget *widget() const { return static_cast<const Widget *>(this); }
};

#endif // CARANGERUNTIMEMIXIN_H
