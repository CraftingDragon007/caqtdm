#ifndef CAWIDGETUPDATEINTERFACE_H
#define CAWIDGETUPDATEINTERFACE_H

#include "knobData.h"

#include <QString>

struct caWidgetUpdateResult
{
    QString operatorError;
    bool logOperatorError = false;
};

// A non-QObject capability: QWidget subclasses keep their existing Qt base class.
class caWidgetUpdateInterface
{
public:
    virtual ~caWidgetUpdateInterface() = default;

    virtual bool needsAggregateAlarm() const = 0;
    virtual bool needsVisibilityUpdate() const = 0;
    virtual caWidgetUpdateResult applyChannelUpdate(const QString &units, const QString &value,
                                                    const knobData &data, short aggregateAlarm) = 0;
};

#endif // CAWIDGETUPDATEINTERFACE_H
