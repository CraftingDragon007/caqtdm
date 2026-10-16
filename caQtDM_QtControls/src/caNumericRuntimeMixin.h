#ifndef CANUMERICRUNTIMEMIXIN_H
#define CANUMERICRUNTIMEMIXIN_H

#include "caWidgetUpdateInterface.h"

#include <QCoreApplication>
#include <QEvent>
#include <QWidget>

#include <cmath>

class caNumericRuntimeInterface : public caWidgetUpdateInterface
{
public:
    virtual int numericObjectType() const = 0;
    virtual QString numericPV() const = 0;
    virtual void setNumericPV(const QString &pv) = 0;
    virtual bool numericIsElevated() = 0;
    virtual void raiseNumeric() = 0;
    virtual void applyNumericLimits(const knobData &data) = 0;
    virtual bool menuUsesUserPrecision() const = 0;
    virtual int menuPrecision() const = 0;
};

template <typename Widget>
class caNumericRuntimeMixin : public caNumericRuntimeInterface
{
public:
    bool needsAggregateAlarm() const override { return false; }
    bool needsVisibilityUpdate() const override { return false; }

    QString numericPV() const override { return widget()->getPV(); }
    void setNumericPV(const QString &pv) override { widget()->setPV(pv); }
    bool numericIsElevated() override { return widget()->isElevated(); }
    void raiseNumeric() override { widget()->raise(); }
    bool menuUsesUserPrecision() const override
    { return widget()->getPrecisionMode() == Widget::User; }
    int menuPrecision() const override { return widget()->decDigits(); }

    caWidgetUpdateResult applyChannelUpdate(const QString &units, const QString &value,
                                            const knobData &data, short aggregateAlarm) override
    {
        Q_UNUSED(units);
        Q_UNUSED(value);
        Q_UNUSED(aggregateAlarm);

        Widget *w = widget();
        caWidgetUpdateResult result;
        if (!data.edata.connected) {
            w->setConnectedColors(false);
            return result;
        }

        applyNumericLimits(data);
        if (preConnectColors()) w->setConnectedColors(true);
        if (w->getColorMode() == Widget::Static || w->getColorMode() == Widget::Default)
            w->setConnectedColors(true);
        else
            w->setAlarmColors(data.edata.severity);

        if (data.edata.fieldtype == caSTRING || data.edata.fieldtype == caCHAR) {
            w->setEnabled(false);
            result.operatorError = QString("%1 %2 does not treat other then numerical values")
                                       .arg(QString::fromLatin1(w->metaObject()->className()), w->objectName());
            result.logOperatorError = logInvalidNumericType();
            return result;
        }

        w->silentSetValue(data.edata.rvalue);
        w->setAccessW(bool(data.edata.accessW));
        if (w->hasFocus())
            QCoreApplication::postEvent(w, new QEvent(QEvent::Enter));
        return result;
    }

    void applyNumericLimits(const knobData &data) override
    {
        if (!data.edata.initialize) return;

        Widget *w = widget();
        double maxValue = 1.0;
        double minValue = 0.0;
        if (w->getLimitsMode() == Widget::Channel) {
            if (data.edata.upper_disp_limit == data.edata.lower_disp_limit ||
                std::fabs(data.edata.upper_disp_limit - data.edata.lower_disp_limit) <= 0.001) {
                maxValue = 100000.0;
                minValue = -100000.0;
            } else {
                maxValue = data.edata.upper_disp_limit;
                minValue = data.edata.lower_disp_limit;
            }
        } else {
            maxValue = w->getMaxValue();
            minValue = w->getMinValue();
        }
        w->setMaxValue(maxValue);
        w->setMinValue(minValue);

        if (w->getFixedFormat()) return;

        double maxAbs = qMax(std::fabs(maxValue), std::fabs(minValue));
        if (qIsFinite(data.edata.rvalue))
            maxAbs = qMax(maxAbs, std::fabs(data.edata.rvalue));

        if (w->getPrecisionMode() == Widget::Channel) {
            int precision = data.edata.precision;
            if (precision < 0) precision = 0;
            if (precision > w->getMaxChannelPrecision())
                precision = w->getMaxChannelPrecision();
            const int width = maxAbs > 1.0
                                  ? int(std::log10(maxAbs)) + 2 + precision
                                  : 2 + precision;
            w->setDigits(width - precision - 1, precision);
        } else {
            const int width = maxAbs > 1.0 ? int(std::log10(maxAbs)) + 2 : 2;
            w->setIntDigits(width - 1);
        }
    }

protected:
    virtual bool preConnectColors() const { return false; }
    virtual bool logInvalidNumericType() const { return true; }

private:
    Widget *widget() { return static_cast<Widget *>(this); }
    const Widget *widget() const { return static_cast<const Widget *>(this); }
};

#endif // CANUMERICRUNTIMEMIXIN_H
