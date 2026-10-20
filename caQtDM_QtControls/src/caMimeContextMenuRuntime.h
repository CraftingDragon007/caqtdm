#ifndef CAMIMECONTEXTMENURUNTIME_H
#define CAMIMECONTEXTMENURUNTIME_H

#include <QStringList>

class caMimeContextMenuRuntime
{
public:
    virtual ~caMimeContextMenuRuntime() = default;
    virtual QStringList contextMenuFiles() const = 0;
};

#endif // CAMIMECONTEXTMENURUNTIME_H
