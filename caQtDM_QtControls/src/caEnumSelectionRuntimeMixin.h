#ifndef CAENUMSELECTIONRUNTIMEMIXIN_H
#define CAENUMSELECTIONRUNTIMEMIXIN_H

#include "caWidgetUpdateInterface.h"
#include "alarmdefs.h"

#include <QCoreApplication>
#include <QEvent>
#include <QList>
#include <QWidget>

class caEnumSelectionRuntime : public caWidgetUpdateInterface
{
public:
    enum SelectionMonitorKind { PrimarySelectionMonitor, MaskSelectionMonitor };

    virtual int selectionObjectType() const = 0;
    virtual QString selectionPV() const = 0;
    virtual void setSelectionPV(const QString &pv) = 0;
    virtual QString selectionMaskPV() const { return QString(); }
    virtual void setSelectionMaskPV(const QString &) {}
    virtual bool selectionUsesPrimarySpec() const = 0;
    virtual QList<SelectionMonitorKind> selectionMonitorKinds() const
    {
        return QList<SelectionMonitorKind>() << PrimarySelectionMonitor;
    }
    // Query one channel at a time so a primary resolved-PV setter can run
    // before the mask request is read, matching the original setup order.
    virtual bool selectionMonitorRequest(SelectionMonitorKind kind, QString &rawPV,
                                         bool &usesPrimarySpec) const
    {
        if (kind == PrimarySelectionMonitor) {
            rawPV = selectionPV();
            usesPrimarySpec = selectionUsesPrimarySpec();
        } else {
            rawPV = selectionMaskPV();
            usesPrimarySpec = false;
        }
        return rawPV.size() > 0;
    }
    virtual void setSelectionMonitorPV(SelectionMonitorKind kind, const QString &pv)
    {
        if (kind == PrimarySelectionMonitor)
            setSelectionPV(pv);
        else
            setSelectionMaskPV(pv);
    }
    virtual void applyMenuAndroidResizeWorkaround() {}
    virtual bool selectionIsElevated() = 0;
    virtual void raiseSelection() = 0;
};

template <typename Widget>
class caEnumSelectionRuntimeMixin : public caEnumSelectionRuntime
{
public:
    bool needsAggregateAlarm() const override { return false; }
    bool needsVisibilityUpdate() const override { return false; }
    QString selectionPV() const override { return widget()->getPV(); }
    void setSelectionPV(const QString &pv) override { widget()->setPV(pv); }
    bool selectionIsElevated() override { return widget()->isElevated(); }
    void raiseSelection() override { widget()->raise(); }

    caWidgetUpdateResult applyChannelUpdate(const QString &units, const QString &value,
                                            const knobData &data, short aggregateAlarm) override
    {
        Q_UNUSED(units);
        Q_UNUSED(aggregateAlarm);
        Widget *w = widget();
        if (data.edata.connected) {
            if (updateSelection(value, data)) {
                if (w->getColorMode() == Widget::Alarm) {
                    w->setAlarmColors(data.edata.severity);
                    afterAlarmColors(data);
                } else if (!w->property("Connect").toBool()) {
                    w->setNormalColors();
                    w->setProperty("Connect", true);
                }
            }
        } else {
            w->setAlarmColors(NOTCONNECTED);
            w->setProperty("Connect", false);
        }
        w->setAccessW(data.edata.accessW);
        if (w->hasFocus())
            QCoreApplication::postEvent(w, new QEvent(QEvent::Enter));
        return {};
    }

protected:
    // Return whether this update should change connected colors.
    virtual bool updateSelection(const QString &value, const knobData &data) = 0;
    virtual void afterAlarmColors(const knobData &) {}

private:
    Widget *widget() { return static_cast<Widget *>(this); }
    const Widget *widget() const { return static_cast<const Widget *>(this); }
};

#endif // CAENUMSELECTIONRUNTIMEMIXIN_H
