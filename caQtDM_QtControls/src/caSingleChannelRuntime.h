#ifndef CASINGLECHANNELRUNTIME_H
#define CASINGLECHANNELRUNTIME_H

#include <QString>

// Small non-QObject contract for widgets whose setup uses one channel.
class caSingleChannelRuntime
{
public:
    virtual ~caSingleChannelRuntime() = default;

    virtual QString singleChannelPV() const = 0;
    virtual bool requestsSingleChannelMonitor() const = 0;
    virtual int singleChannelObjectType() const = 0;
    virtual void setSingleChannelPV(const QString &pv) = 0;
    virtual void prepareSingleChannelMonitor() = 0;
};

#endif // CASINGLECHANNELRUNTIME_H
