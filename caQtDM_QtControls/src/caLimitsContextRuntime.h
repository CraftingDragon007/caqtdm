#ifndef CALIMITSCONTEXTRUNTIME_H
#define CALIMITSCONTEXTRUNTIME_H

#include "knobData.h"

// Facts reported by the generic Get Info action for widgets with limits
// settings. The dispatcher supplies the first monitor because it owns lookup.
struct caLimitsContextState
{
    bool limitsDefault = false;
    bool precMode = false;
    bool limitsMode = false;
    bool highLimit = false;
    bool lowLimit = false;
    int precision = 0;
    double limitsMax = 0.0;
    double limitsMin = 0.0;
    bool hasColorMode = false;
    bool alarmColorMode = false;
};

class caLimitsContextRuntime
{
public:
    virtual ~caLimitsContextRuntime() {}
    virtual caLimitsContextState limitsContextState(const knobData *firstMonitor) const
    {
        (void) firstMonitor;
        return {};
    }
};

#endif // CALIMITSCONTEXTRUNTIME_H
