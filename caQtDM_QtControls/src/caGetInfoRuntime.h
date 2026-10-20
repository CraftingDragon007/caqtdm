#ifndef CAGETINFORUNTIME_H
#define CAGETINFORUNTIME_H

#include <QString>

// Widget-owned facts shown by the generic Get Info context action.
class caGetInfoRuntime
{
public:
    virtual ~caGetInfoRuntime() = default;
    virtual bool getInfoHasVariable() const { return false; }
    virtual QString getInfoVariable() const { return QString(); }
    virtual QString getInfoCalculation() const { return QString(); }
    virtual QString getInfoImageCalculation() const { return QString(); }
    virtual bool getInfoCalculationIsWidgetCalculation() const { return false; }
};

#endif // CAGETINFORUNTIME_H
