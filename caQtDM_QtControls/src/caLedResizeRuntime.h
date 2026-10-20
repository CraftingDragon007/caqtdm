#ifndef CALEDRESIZERUNTIME_H
#define CALEDRESIZERUNTIME_H

class caLedResizeRuntime
{
public:
    virtual ~caLedResizeRuntime() = default;
    virtual void captureResizeBaseline() = 0;
    virtual void resizeLed(double factX, double factY) = 0;
};

#endif // CALEDRESIZERUNTIME_H
