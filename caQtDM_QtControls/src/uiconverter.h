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
 *
 *  General factory for the on-the-fly converters of non-.ui panel
 *  descriptions (.prc, .adl, .edl, ...). The factory dispatches on the
 *  file extension and - where several implementations exist - selects
 *  the converter via settings:
 *    generic scheme: CAQTDM_CONVERTER_<EXT> (e.g. CAQTDM_CONVERTER_PRC=new)
 *    compatibility:  CAQTDM_PRC_CONVERTER (unset/0 = old, 1 = new)
 *  The selection source is encapsulated here, so it can later be moved
 *  to QSettings or an automatic choice without touching the call sites.
 */

#ifndef UICONVERTER_H
#define UICONVERTER_H

#include <qtcontrols_global.h>
#include <QString>

class QWidget;

class QTCON_EXPORT UiConverterInterface
{
public:
    virtual ~UiConverterInterface() {}
    virtual QWidget *load(QWidget *parent) = 0;         // re-callable (macro loop)
    virtual QString title() const { return QString(); } // #!title of the new prc parser
    virtual bool ok() const { return true; }
    virtual QString errorString() const { return QString(); }
};

class QTCON_EXPORT UiConverterFactory
{
public:
    // true when the file needs a converter (known non-.ui extension)
    static bool handles(const QString &fileName);
    static UiConverterInterface *create(const QString &fileName, bool willPrint = false);
    // settings lookup, empty = default implementation
    static QString selectionFor(const QString &extension);
};

#endif // UICONVERTER_H
