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
 *  Copyright (c) 2010 - 2014
 *
 *  Author:
 *    Anton Mezger
 *  Contact details:
 *    anton.mezger@psi.ch
 */

#ifndef PLUGIN_XML_HELPER_H
#define PLUGIN_XML_HELPER_H

#include <QString>
#include <cstring>

typedef char strng[40];
typedef char longtext[500];

static QString XmlFunc(const char *clss, const char *name, int x, int y, int w, int h,
                strng *propertyname, strng* propertytype, longtext *propertytext, int nb)
{
#ifndef DESIGNER_TOOLTIP_DESCRIPTIONS
    Q_UNUSED(propertytext);
#endif
    QString mess = "";
    QString strng1 = "";
    QString strng2 = "";

    // Special handling for cadoubletabwidget
    bool isDoubleTabWidget = (strstr(name, "cadoubletabwidget") != (char*) Q_NULLPTR);

    mess = "<ui language=\"c++\"><widget class=\"%1\" name=\"%2\">\
            <property name=\"geometry\">\
            <rect>\
            <x>%3</x>\
            <y>%4</y>\
            <width>%5</width>\
            <height>%6</height>\
            </rect>\
            </property>\
            </widget>";

    mess = mess.arg(clss).arg(name).arg(x).arg(y).arg(w).arg(h);

    if(nb > 0) {
        if(isDoubleTabWidget) {
            strng1 = " <customwidgets><customwidget><class>%1</class><addpagemethod>addPage</addpagemethod><propertyspecifications>";
        } else {
            strng1 = " <customwidgets><customwidget><class>%1</class><propertyspecifications>";
        }
        strng1 = strng1.arg(clss);
        
        for(int i=0; i<nb; i++) {
#ifdef DESIGNER_TOOLTIP_DESCRIPTIONS
            if(!isDoubleTabWidget) {
                QString strng3 = "<tooltip name=\"%1\">%2</tooltip>";
                strng3 = strng3.arg(propertyname[i]).arg(propertytext[i]);
                strng1.append(strng3);
            }
#endif
            if(strstr(propertytype[i], "multiline") != (char*) Q_NULLPTR) {
                strng2 = " <stringpropertyspecification name=\"%1\" notr=\"true\" type=\"%2\"/>";
                strng2 = strng2.arg(propertyname[i]).arg(propertytype[i]);
            }
            strng1.append(strng2);
        }
        strng1.append(" </propertyspecifications></customwidget></customwidgets>");
    }
    mess.append(strng1);
    mess.append("</ui>");

    //control output in formatted xml format
/*
    QString formattedOutput;
    QDomDocument doc;
    doc.setContent(mess, false);
    QTextStream writer(&formattedOutput);
    doc.save(writer, 4);
    qDebug() << formattedOutput;
*/
    return mess;
}

#endif // PLUGIN_XML_HELPER_H
