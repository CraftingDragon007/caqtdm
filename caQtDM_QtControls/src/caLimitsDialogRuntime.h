#ifndef CALIMITSDIALOGRUNTIME_H
#define CALIMITSDIALOGRUNTIME_H

#include <QString>

// Runtime behavior shared by widgets that expose the limits dialog.
// The channel values are supplied by the dialog because it owns monitor lookup.
struct caLimitsDialogState
{
    bool userLimits = false;
    double minValue = 0.0;
    double maxValue = 0.0;
    bool userPrecision = false;
    int precision = 0;
};

struct caLimitsDialogApply
{
    bool userLimits = false;
    double minValue = 0.0;
    double maxValue = 0.0;
    double channelLowerLimit = 0.0;
    double channelUpperLimit = 0.0;
    bool channelLimitsAvailable = false;
    bool userPrecision = false;
    int channelPrecision = 0;
    int precision = 0;
};

class caLimitsDialogRuntime
{
public:
    virtual ~caLimitsDialogRuntime() {}
    virtual QString limitsDialogPV() const = 0;
    virtual caLimitsDialogState limitsDialogState(int channelPrecision) const = 0;
    virtual void applyLimitsDialog(const caLimitsDialogApply &state) = 0;
};

#endif // CALIMITSDIALOGRUNTIME_H
