#ifndef CACONTEXTCOLORMODERUNTIME_H
#define CACONTEXTCOLORMODERUNTIME_H

// Widgets expose their alarm/static context-menu color mode without making
// the dispatcher depend on their concrete color-mode enums.
class caContextColorModeRuntime
{
public:
    virtual ~caContextColorModeRuntime() {}
    virtual bool contextUsesAlarmColorMode() const = 0;
};

#endif // CACONTEXTCOLORMODERUNTIME_H
