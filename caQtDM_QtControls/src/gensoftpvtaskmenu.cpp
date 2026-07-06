/*
 *  This file is part of the caQtDM Framework, developed at the Paul Scherrer Institut,
 *  Villigen, Switzerland
 *
 *  The caQtDM Framework is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  The caQtDM Framework is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the caQtDM Framework.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2010 - 2026
 *
 *  Author:
 *    Helge Brands
 *  Contact details:
 *    helge.brands@psi.ch
 */

#include <QtDesigner/QtDesigner>

#include "gensoftpvtaskmenu.h"
#include "gensoftpvdialog.h"
#include "gensoftpv.h"

GenSoftPVTaskMenu::GenSoftPVTaskMenu(QWidget *widget, QObject *parent) : QObject(parent)
{
    softpvWidget = widget;
    editStateAction = new QAction(tr("Edit soft PV..."), this);
    connect(editStateAction, SIGNAL(triggered()), this, SLOT(editState()));
}

void GenSoftPVTaskMenu::editState()
{
    GenSoftPVDialog dialog(softpvWidget);
    dialog.exec();
}

QAction *GenSoftPVTaskMenu::preferredEditAction() const
{
    return editStateAction;
}

QList<QAction *> GenSoftPVTaskMenu::taskActions() const
{
    QList<QAction *> list;
    list.append(editStateAction);
    return list;
}

GenSoftPVTaskMenuFactory::GenSoftPVTaskMenuFactory(QExtensionManager *parent) : QExtensionFactory(parent)
{
}

QObject *GenSoftPVTaskMenuFactory::createExtension(QObject *object, const QString &iid, QObject *parent) const
{
    if(iid != Q_TYPEID(QDesignerTaskMenuExtension)) return 0;

    if(genSoftPV *widget = qobject_cast<genSoftPV *>(object)) {
        return new GenSoftPVTaskMenu(widget, parent);
    }
    return 0;
}
