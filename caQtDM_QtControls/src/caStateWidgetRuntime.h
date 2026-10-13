#ifndef CASTATEWIDGETRUNTIME_H
#define CASTATEWIDGETRUNTIME_H

#include "caWidgetUpdateInterface.h"

#include <Qt>

// Shared monitor declaration and value decoding for the LED and toggle control.
class caStateWidgetRuntime : public caWidgetUpdateInterface
{
public:
    bool needsAggregateAlarm() const override { return false; }
    bool needsVisibilityUpdate() const override { return false; }

    virtual int stateObjectType() const = 0;
    virtual QString statePV() const = 0;
    virtual void setStatePV(const QString &pv) = 0;
    virtual bool monitorEmptyPV() const = 0;

protected:
    static Qt::CheckState decodeState(const QString &trueValue, const QString &falseValue,
                                      const knobData &data, const QString &value);
};

#endif // CASTATEWIDGETRUNTIME_H
