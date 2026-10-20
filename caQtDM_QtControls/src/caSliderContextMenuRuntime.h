#ifndef CASLIDERCONTEXTMENURUNTIME_H
#define CASLIDERCONTEXTMENURUNTIME_H

#include <QMenu>
#include <QString>

class caSliderContextMenuRuntime
{
public:
    virtual ~caSliderContextMenuRuntime() = default;

    virtual void addSliderContextActions(QMenu &menu, const QString &getInfoLabel,
                                         const QString &changeValueLabel) const = 0;
};

#endif // CASLIDERCONTEXTMENURUNTIME_H
