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
 *    Erik Schwarz
 *  Contact details:
 *    erik.schwarz@psi.ch
 */

#ifndef FAKEFILEOPENWINDOW_H
#define FAKEFILEOPENWINDOW_H

#include <QMainWindow>
#include <QDebug>

class FakeFileOpenWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FakeFileOpenWindow(QWidget *parent = Q_NULLPTR)
        : QMainWindow(parent)
    {}

public slots:
    void Callback_IosExit() { qInfo() << "Callback_IosExit"; }
    void Callback_ReloadWindow(QWidget *) { qInfo() << "Callback_ReloadWindow"; }
    void Callback_ReloadAllWindows() { qInfo() << "Callback_ReloadAllWindows"; }
    void Callback_OpenNewFile(const QString &, const QString &, const QString &, const QString &)
    {
        qInfo() << "Callback_OpenNewFile";
    }

signals:
    void themeChanged();
};

#endif // FAKEFILEOPENWINDOW_H
