#ifndef CAPLOTCONTEXTMENURUNTIME_H
#define CAPLOTCONTEXTMENURUNTIME_H

#include <QMenu>
#include <QString>

class caPlotContextMenuRuntime
{
public:
    virtual ~caPlotContextMenuRuntime() = default;
    virtual void addPlotContextActions(QMenu &menu, const QString &changeAxisLabel,
                                       const QString &resetZoomLabel,
                                       const QString &copyImageLabel) const = 0;
};

#endif // CAPLOTCONTEXTMENURUNTIME_H
