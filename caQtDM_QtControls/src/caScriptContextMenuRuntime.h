#ifndef CASCRIPTCONTEXTMENURUNTIME_H
#define CASCRIPTCONTEXTMENURUNTIME_H

#include <QMenu>
#include <QString>

class caScriptContextMenuRuntime
{
public:
    virtual ~caScriptContextMenuRuntime() = default;
    virtual void addScriptContextActions(QMenu &menu, const QString &killProcessLabel) const = 0;
    virtual void *contextMenuProcess() const = 0;
};

#endif // CASCRIPTCONTEXTMENURUNTIME_H
