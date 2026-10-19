#ifndef CAWAVETABLECONTEXTMENURUNTIME_H
#define CAWAVETABLECONTEXTMENURUNTIME_H

#include <QMenu>
#include <QString>

class caWaveTableContextMenuRuntime
{
public:
    virtual ~caWaveTableContextMenuRuntime() = default;
    virtual void addWaveTableContextActions(QMenu &menu, const QString &infoLabel,
                                            const QString &copyDataLabel,
                                            const QString &pasteDataLabel) = 0;
    virtual bool handleWaveTableContextAction(const QString &actionText,
                                              const QString &copyDataLabel,
                                              const QString &pasteDataLabel) = 0;
};

#endif // CAWAVETABLECONTEXTMENURUNTIME_H
