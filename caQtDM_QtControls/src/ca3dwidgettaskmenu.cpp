/*
 *  This file is part of the caQtDM Framework.
 */

#include <QtDesigner/QtDesigner>
#include <QActionGroup>
#include <QMenu>
#include "ca3dwidget.h"
#include "ca3dconfigdialog.h"
#include "ca3dwidgettaskmenu.h"

ca3DWidgetTaskMenu::ca3DWidgetTaskMenu(ca3DWidget *widget, QObject *parent)
    : QObject(parent)
    , editSceneAction(new QAction(tr("Edit 3D Scene..."), this))
    , renderTierAction(new QAction(tr("Rendering Quality"), this))
    , renderTierMenu(new QMenu(widget))
    , widget3D(widget)
{
    connect(editSceneAction, SIGNAL(triggered()), this, SLOT(editScene()));

    QActionGroup *renderTierGroup = new QActionGroup(this);
    renderTierGroup->setExclusive(true);
    QAction *hardwareAction = renderTierMenu->addAction(tr("Hardware"));
    hardwareAction->setCheckable(true);
    renderTierGroup->addAction(hardwareAction);
    connect(hardwareAction, &QAction::triggered, this, [this]() {
        widget3D->setRenderTier(ca3DWidget::RenderTierHardware);
    });
    QAction *softwareAction = renderTierMenu->addAction(tr("Software"));
    softwareAction->setCheckable(true);
    renderTierGroup->addAction(softwareAction);
    connect(softwareAction, &QAction::triggered, this, [this]() {
        widget3D->setRenderTier(ca3DWidget::RenderTierSoftware);
    });
    QAction *fallbackAction = renderTierMenu->addAction(tr("Fallback (2D)"));
    fallbackAction->setCheckable(true);
    renderTierGroup->addAction(fallbackAction);
    connect(fallbackAction, &QAction::triggered, this, [this]() {
        widget3D->setRenderTier(ca3DWidget::RenderTierFallback);
    });

    const int tier = widget3D->getRenderTier();
    hardwareAction->setChecked(tier == ca3DWidget::RenderTierHardware);
    softwareAction->setChecked(tier == ca3DWidget::RenderTierSoftware);
    fallbackAction->setChecked(tier == ca3DWidget::RenderTierFallback);
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
    renderTierAction->setMenu(renderTierMenu);
    return {editSceneAction, renderTierAction};
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
