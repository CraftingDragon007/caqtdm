#ifndef CAVISIBILITYMONITORINTERFACE_H
#define CAVISIBILITYMONITORINTERFACE_H

#include <QString>

// Widget-owned visibility settings. The dispatcher still creates and owns monitors.
class caVisibilityMonitorInterface
{
public:
    enum Mode { Static = 0, IfNotZero = 1, IfZero = 2, Calc = 3 };

    virtual ~caVisibilityMonitorInterface() = default;
    virtual Mode visibilityMode() const = 0;
    virtual bool skipVisibilityMonitors() const = 0;
    virtual QString visibilityChannel(int index) const = 0;
    virtual void setVisibilityChannel(int index, const QString &pv) = 0;
    virtual QString visibilityExpression() const = 0;
    virtual void setVisibilityExpression(const QString &expression) = 0;
    virtual bool applyVisibilityResult(bool visible) { (void)visible; return false; }
};

#endif // CAVISIBILITYMONITORINTERFACE_H
