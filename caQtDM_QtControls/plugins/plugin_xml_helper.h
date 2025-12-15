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
 *  Copyright (c) 2010 - 2025
 *
 *  Authors:
 *    Anton Mezger
 *    Julian Houba
 *  Contact details:
 *    anton.mezger@psi.ch
 *    julian.houba@psi.ch
 */

#ifndef PLUGIN_XML_HELPER_H
#define PLUGIN_XML_HELPER_H

#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <cstring>

typedef char strng[40];
typedef char longtext[500];

static QString XmlFunc(const char *clss, const char *name, int x, int y, int w, int h,
                strng *propertyname, strng* propertytype, longtext *propertytext, int nb)
{
#ifndef DESIGNER_TOOLTIP_DESCRIPTIONS
    Q_UNUSED(propertytext);
#endif

    QDomDocument doc;
    QDomElement uiElement = doc.createElement("ui");
    uiElement.setAttribute("language", "c++");
    doc.appendChild(uiElement);

    QDomElement widgetElement = doc.createElement("widget");
    widgetElement.setAttribute("class", clss);
    widgetElement.setAttribute("name", name);
    uiElement.appendChild(widgetElement);

    QDomElement geometryProp = doc.createElement("property");
    geometryProp.setAttribute("name", "geometry");
    widgetElement.appendChild(geometryProp);

    QDomElement rect = doc.createElement("rect");
    geometryProp.appendChild(rect);

    QDomElement xElem = doc.createElement("x");
    xElem.appendChild(doc.createTextNode(QString::number(x)));
    rect.appendChild(xElem);

    QDomElement yElem = doc.createElement("y");
    yElem.appendChild(doc.createTextNode(QString::number(y)));
    rect.appendChild(yElem);

    QDomElement widthElem = doc.createElement("width");
    widthElem.appendChild(doc.createTextNode(QString::number(w)));
    rect.appendChild(widthElem);

    QDomElement heightElem = doc.createElement("height");
    heightElem.appendChild(doc.createTextNode(QString::number(h)));
    rect.appendChild(heightElem);

    // Add custom widgets section if properties are specified
    if(nb > 0) {
        QDomElement customWidgets = doc.createElement("customwidgets");
        uiElement.appendChild(customWidgets);

        QDomElement customWidget = doc.createElement("customwidget");
        customWidgets.appendChild(customWidget);

        QDomElement classElem = doc.createElement("class");
        classElem.appendChild(doc.createTextNode(clss));
        customWidget.appendChild(classElem);

        // Special handling for cadoubletabwidget
        bool isDoubleTabWidget = (strstr(name, "cadoubletabwidget") != (char*) Q_NULLPTR);
        if(isDoubleTabWidget) {
            QDomElement addPageMethod = doc.createElement("addpagemethod");
            addPageMethod.appendChild(doc.createTextNode("addPage"));
            customWidget.appendChild(addPageMethod);
        }

        QDomElement propertySpecs = doc.createElement("propertyspecifications");
        customWidget.appendChild(propertySpecs);

        for(int i=0; i<nb; i++) {
#ifdef DESIGNER_TOOLTIP_DESCRIPTIONS
            if(!isDoubleTabWidget) {
                QDomElement tooltip = doc.createElement("tooltip");
                tooltip.setAttribute("name", propertyname[i]);
                tooltip.appendChild(doc.createTextNode(propertytext[i]));
                propertySpecs.appendChild(tooltip);
            }
#endif
            if(strstr(propertytype[i], "multiline") != (char*) Q_NULLPTR) {
                QDomElement stringPropSpec = doc.createElement("stringpropertyspecification");
                stringPropSpec.setAttribute("name", propertyname[i]);
                stringPropSpec.setAttribute("notr", "true");
                stringPropSpec.setAttribute("type", propertytype[i]);
                propertySpecs.appendChild(stringPropSpec);
            }
        }
    }

    // Convert to string without XML declaration and indentation
    return doc.toString(-1);
}

#endif // PLUGIN_XML_HELPER_H
