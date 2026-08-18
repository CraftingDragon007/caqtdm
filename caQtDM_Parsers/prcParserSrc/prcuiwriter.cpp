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

#include "prcuiwriter.h"

PrcUiWriter::PrcUiWriter(QIODevice *device) : xml(device)
{
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(1);
}

void PrcUiWriter::startDocument()
{
    xml.writeStartDocument();
    xml.writeStartElement("ui");
    xml.writeAttribute("version", "4.0");
    xml.writeTextElement("class", "MainWindow");
}

void PrcUiWriter::endDocument()
{
    xml.writeEndElement(); // ui
    xml.writeEndDocument();
}

void PrcUiWriter::startWidget(const QString &klass, const QString &name)
{
    xml.writeStartElement("widget");
    xml.writeAttribute("class", klass);
    xml.writeAttribute("name", name);
}

void PrcUiWriter::endWidget()
{
    xml.writeEndElement();
}

void PrcUiWriter::startLayout(const QString &klass, const QString &name)
{
    xml.writeStartElement("layout");
    xml.writeAttribute("class", klass);
    xml.writeAttribute("name", name);
}

void PrcUiWriter::endLayout()
{
    xml.writeEndElement();
}

void PrcUiWriter::startItem(int row, int column, int colspan)
{
    xml.writeStartElement("item");
    xml.writeAttribute("row", QString::number(row));
    xml.writeAttribute("column", QString::number(column));
    if(colspan > 1) xml.writeAttribute("colspan", QString::number(colspan));
}

void PrcUiWriter::startItem()
{
    xml.writeStartElement("item");
}

void PrcUiWriter::endItem()
{
    xml.writeEndElement();
}

void PrcUiWriter::stringProperty(const QString &name, const QString &value, bool notr)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeStartElement("string");
    if(notr) xml.writeAttribute("notr", "true");
    xml.writeCharacters(value);
    xml.writeEndElement();
    xml.writeEndElement();
}

void PrcUiWriter::enumProperty(const QString &name, const QString &value)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeTextElement("enum", value);
    xml.writeEndElement();
}

void PrcUiWriter::setProperty(const QString &name, const QString &value)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeTextElement("set", value);
    xml.writeEndElement();
}

void PrcUiWriter::numberProperty(const QString &name, int value)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeTextElement("number", QString::number(value));
    xml.writeEndElement();
}

void PrcUiWriter::boolProperty(const QString &name, bool value)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeTextElement("bool", value ? "true" : "false");
    xml.writeEndElement();
}

void PrcUiWriter::colorProperty(const QString &name, const QColor &color, int alpha)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeStartElement("color");
    xml.writeAttribute("alpha", QString::number(alpha >= 0 ? alpha : color.alpha()));
    xml.writeTextElement("red", QString::number(color.red()));
    xml.writeTextElement("green", QString::number(color.green()));
    xml.writeTextElement("blue", QString::number(color.blue()));
    xml.writeEndElement();
    xml.writeEndElement();
}

void PrcUiWriter::sizeProperty(const QString &name, int width, int height)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeStartElement("size");
    xml.writeTextElement("width", QString::number(width));
    xml.writeTextElement("height", QString::number(height));
    xml.writeEndElement();
    xml.writeEndElement();
}

void PrcUiWriter::rectProperty(const QString &name, int x, int y, int width, int height)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", name);
    xml.writeStartElement("rect");
    xml.writeTextElement("x", QString::number(x));
    xml.writeTextElement("y", QString::number(y));
    xml.writeTextElement("width", QString::number(width));
    xml.writeTextElement("height", QString::number(height));
    xml.writeEndElement();
    xml.writeEndElement();
}

void PrcUiWriter::fontProperty(const QString &family, int pointSize)
{
    xml.writeStartElement("property");
    xml.writeAttribute("name", "font");
    xml.writeStartElement("font");
    xml.writeTextElement("family", family);
    xml.writeTextElement("pointsize", QString::number(pointSize));
    xml.writeEndElement();
    xml.writeEndElement();
}

void PrcUiWriter::zOrder(const QString &name)
{
    xml.writeTextElement("zorder", name);
}
