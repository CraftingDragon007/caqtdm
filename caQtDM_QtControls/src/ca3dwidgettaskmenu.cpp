/*
 *  This file is part of the caQtDM Framework.
 */

#include <QtDesigner/QtDesigner>
#include "ca3dwidget.h"
#include "ca3dconfigdialog.h"
#include "ca3dwidgettaskmenu.h"

ca3DWidgetTaskMenu::ca3DWidgetTaskMenu(ca3DWidget *widget, QObject *parent)
    : QObject(parent)
    , editSceneAction(new QAction(tr("Edit 3D Scene..."), this))
    , widget3D(widget)
{
    connect(editSceneAction, SIGNAL(triggered()), this, SLOT(editScene()));
}

void ca3DWidgetTaskMenu::editScene()
{
    ca3DConfigDialog dialog(widget3D, Q_NULLPTR);
    dialog.exec();
}

QAction *ca3DWidgetTaskMenu::preferredEditAction() const
{
    return editSceneAction;
}

QList<QAction *> ca3DWidgetTaskMenu::taskActions() const
{
    QList<QAction *> list;
    list.append(editSceneAction);
    return list;
}

ca3DWidgetTaskMenuFactory::ca3DWidgetTaskMenuFactory(QExtensionManager *parent)
    : QExtensionFactory(parent)
{
}

QObject *ca3DWidgetTaskMenuFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if (iid != Q_TYPEID(QDesignerTaskMenuExtension)) {
        return nullptr;
    }

    if (ca3DWidget *widget = qobject_cast<ca3DWidget *>(object)) {
        return new ca3DWidgetTaskMenu(widget, parent);
    }

    return nullptr;
}
