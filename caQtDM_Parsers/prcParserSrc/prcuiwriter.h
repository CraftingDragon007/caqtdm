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

#ifndef PRCUIWRITER_H
#define PRCUIWRITER_H

#include <QXmlStreamWriter>
#include <QColor>
#include <QString>
#include "prcparserdefs.h"

// thin helper around QXmlStreamWriter for Qt designer .ui documents
class PRCPARSER_EXPORT PrcUiWriter
{
public:
    explicit PrcUiWriter(QIODevice *device);

    void startDocument();                        // <ui version="4.0"><class>…
    void endDocument();                          // closes everything

    void startWidget(const QString &klass, const QString &name);
    void endWidget();
    void startLayout(const QString &klass, const QString &name);
    void endLayout();
    void startItem(int row, int column, int colspan = 1);   // grid item
    void startItem();                                       // box layout item
    void endItem();

    // properties
    void stringProperty(const QString &name, const QString &value, bool notr = false);
    void enumProperty(const QString &name, const QString &value);
    void setProperty(const QString &name, const QString &value);
    void numberProperty(const QString &name, int value);
    void boolProperty(const QString &name, bool value);
    void colorProperty(const QString &name, const QColor &color, int alpha = -1);
    void sizeProperty(const QString &name, int width, int height);
    void rectProperty(const QString &name, int x, int y, int width, int height);
    void fontProperty(const QString &family, int pointSize);
    void zOrder(const QString &name);

    QXmlStreamWriter &raw() { return xml; }

private:
    QXmlStreamWriter xml;
};

#endif // PRCUIWRITER_H
