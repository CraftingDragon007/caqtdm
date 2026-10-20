#ifndef CARESIZEFACTORRUNTIME_H
#define CARESIZEFACTORRUNTIME_H

class caResizeFactorRuntime
{
public:
    virtual ~caResizeFactorRuntime() = default;
    virtual void setResizeFactors(double factX, double factY) = 0;
};

#endif // CARESIZEFACTORRUNTIME_H
