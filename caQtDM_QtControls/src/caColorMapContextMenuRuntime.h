#ifndef CACOLORMAPCONTEXTMENURUNTIME_H
#define CACOLORMAPCONTEXTMENURUNTIME_H

#include <QMenu>
#include <QString>
#include <QStringList>

class caColorMapContextMenuRuntime
{
public:
    enum Action {
        Unhandled,
        ToggleFitToSize,
        Wavelength,
        Hot,
        Heat,
        Jet,
        Custom,
        Grey,
        AsIs
    };

    virtual ~caColorMapContextMenuRuntime() = default;
    virtual void addFitToSizeContextAction(QMenu &menu, const QString &fitToSizeLabel) const = 0;
    virtual void addColorMapContextActions(QMenu &menu, const QStringList &labels) const = 0;
    virtual bool handleColorMapContextAction(const QString &text, const QStringList &labels,
                                             const QString &fitToSizeLabel) = 0;

    static QStringList colorMapLabels(const QString &setAsIs, const QString &setGrey,
                                      const QString &wavelength,
                                      const QString &hot, const QString &heat,
                                      const QString &jet, const QString &custom)
    {
        QStringList labels;
        labels << setAsIs << setGrey << wavelength << hot << heat << jet << custom;
        return labels;
    }

    static void addFitToSizeAction(QMenu &menu, const QString &fitToSizeLabel,
                                   bool fitToSizeEnabled, bool fitToSizeChecked)
    {
        if (!fitToSizeEnabled) return;
        QAction *action = menu.addAction(fitToSizeLabel);
        action->setCheckable(true);
        action->setChecked(fitToSizeChecked);
    }

    static void addColorMapActions(QMenu &menu, const QStringList &labels)
    {
        for (const QString &label : labels)
            menu.addAction(label);
    }

    static Action resolveAction(const QString &text, const QStringList &labels,
                                const QString &fitToSizeLabel)
    {
        // Keep the legacy text-contains checks and their priority. This matters
        // for environment-provided actions whose text happens to contain a key.
        if (!fitToSizeLabel.isEmpty() && text.contains(fitToSizeLabel)) return ToggleFitToSize;
        if (labels.size() > 2 && text.contains(labels.at(2))) return Wavelength;
        if (labels.size() > 3 && text.contains(labels.at(3))) return Hot;
        if (labels.size() > 4 && text.contains(labels.at(4))) return Heat;
        if (labels.size() > 5 && text.contains(labels.at(5))) return Jet;
        if (labels.size() > 6 && text.contains(labels.at(6))) return Custom;
        if (labels.size() > 1 && text.contains(labels.at(1))) return Grey;
        if (!labels.isEmpty() && text.contains(labels.at(0))) return AsIs;
        return Unhandled;
    }
};

#endif // CACOLORMAPCONTEXTMENURUNTIME_H
