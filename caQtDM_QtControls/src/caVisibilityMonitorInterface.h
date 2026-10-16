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

    QString visibilityDescription() const
    {
        switch (visibilityMode()) {
        case Static: return QStringLiteral("static");
        case IfNotZero: return QStringLiteral("IfNotZero");
        case IfZero: return QStringLiteral("IfZero");
        case Calc: return visibilityExpression();
        }
        return QString();
    }
};

template <typename Widget>
class caVisibilityRuntimeMixin : public caVisibilityMonitorInterface
{
public:
    Mode visibilityMode() const override
    { return static_cast<Mode>(widget()->getVisibility()); }
    bool skipVisibilityMonitors() const override { return false; }
    QString visibilityChannel(int index) const override
    {
        switch (index) {
        case 0: return widget()->getChannelA();
        case 1: return widget()->getChannelB();
        case 2: return widget()->getChannelC();
        case 3: return widget()->getChannelD();
        default: return QString();
        }
    }
    void setVisibilityChannel(int index, const QString &pv) override
    {
        switch (index) {
        case 0: widget()->setChannelA(pv); break;
        case 1: widget()->setChannelB(pv); break;
        case 2: widget()->setChannelC(pv); break;
        case 3: widget()->setChannelD(pv); break;
        }
    }
    QString visibilityExpression() const override { return widget()->getVisibilityCalc(); }
    void setVisibilityExpression(const QString &expression) override
    { widget()->setVisibilityCalc(expression); }

private:
    Widget *widget() { return static_cast<Widget *>(this); }
    const Widget *widget() const { return static_cast<const Widget *>(this); }
};

#endif // CAVISIBILITYMONITORINTERFACE_H
