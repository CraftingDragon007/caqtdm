#ifndef CATEXTWIDGETRUNTIME_H
#define CATEXTWIDGETRUNTIME_H

#include <QString>

class QWidget;

// Runtime setup shared by the text entry, line edit, and multiline widgets.
// PV lookup and monitor registration remain the library's responsibility.
class caTextWidgetRuntime
{
public:
    virtual ~caTextWidgetRuntime() = default;

    virtual QString runtimeTextPV() const = 0;
    virtual void setRuntimeTextPV(const QString &pv) = 0;
    // Optional monitor declared from the resolved primary PV (for example, line-edit units).
    virtual bool supportsRuntimeSecondaryMonitor() const { return false; }
    virtual bool runtimeSecondaryMonitor(const QString &, QString &, int &) const { return false; }
    virtual QString runtimeInitialText() const = 0;
    virtual void prepareRuntimeTextMonitor() = 0;
    virtual bool resetsRuntimeTextMonitorSpecData() const { return false; }
    virtual bool expandsRuntimeInitialText() const = 0;
    virtual bool reaffectsRuntimeToolTips() const = 0;

    virtual void beginRuntimeTextSetup() = 0;
    virtual void setRuntimeText(const QString &text) = 0;
    virtual void setRuntimeToolTip(const QString &text) = 0;
    virtual void setRuntimeStatusTip(const QString &text) = 0;
    virtual void finishRuntimeTextSetup(QWidget *display) = 0;
};

#endif // CATEXTWIDGETRUNTIME_H
