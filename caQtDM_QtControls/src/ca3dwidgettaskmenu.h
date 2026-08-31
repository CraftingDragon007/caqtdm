/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DWIDGETTASKMENU_H
#define CA3DWIDGETTASKMENU_H

#include <QtDesigner/QDesignerTaskMenuExtension>
#include <QtDesigner/QExtensionFactory>
#include <qtcontrols_global.h>
#include <QAction>

QT_BEGIN_NAMESPACE
class QAction;
class QExtensionManager;
class QMenu;
QT_END_NAMESPACE
class ca3DWidget;

class QTCON_EXPORT ca3DWidgetTaskMenu : public QObject, public QDesignerTaskMenuExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerTaskMenuExtension)

public:
    ca3DWidgetTaskMenu(ca3DWidget *widget, QObject *parent);

    QAction *preferredEditAction() const;
    QList<QAction *> taskActions() const;

private slots:
    void editScene();

private:
    QAction *editSceneAction;
    QAction *renderTierAction;
    QMenu *renderTierMenu;
    ca3DWidget *widget3D;
};

class QTCON_EXPORT ca3DWidgetTaskMenuFactory : public QExtensionFactory
{
    Q_OBJECT

public:
    explicit ca3DWidgetTaskMenuFactory(QExtensionManager *parent = nullptr);

protected:
    QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

#endif
