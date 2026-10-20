#ifndef CATEXTENTRYCONTEXTMENURUNTIME_H
#define CATEXTENTRYCONTEXTMENURUNTIME_H

#include <QMenu>
#include <QString>

class caTextEntryContextMenuRuntime
{
public:
    virtual ~caTextEntryContextMenuRuntime() = default;

    // The library uses this gate before looking up MonitorList.at(1).
    virtual bool contextMenuWriteAccess() const = 0;
    virtual void addTextEntryContextActions(QMenu &menu, const QString &inputDialogLabel,
                                            const QString &fileDialogLabel, bool hasValidPv,
                                            bool pvSupportsFileDialog, bool vncServer) const = 0;
};

#endif // CATEXTENTRYCONTEXTMENURUNTIME_H
