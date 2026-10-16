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

#include "caled.h"
#include "alarmdefs.h"
#include "qtdefinitions.h"

Q_LOGGING_CATEGORY(caLedLog, "caqtdm.widgets.caled")

int caLed::stateObjectType() const { return caLed_Widget; }

caWidgetUpdateResult caLed::applyChannelUpdate(const QString &units, const QString &value,
                                               const knobData &data, short aggregateAlarm)
{
    Q_UNUSED(units);
    Q_UNUSED(aggregateAlarm);
    qCDebug(caLedLog) << "led" << objectName();
    if (!data.edata.connected) {
        setAlarmColors(NOTCONNECTED);
    } else if (getColorMode() == Static) {
        setState(decodeState(getTrueValue(), getFalseValue(), data, value));
    } else {
        setAlarmColors(data.edata.severity);
    }
    return {};
}

void caLed::captureResizeBaseline()
{
    originalLedWidth = ledWidth();
    originalLedHeight = ledHeight();
}

void caLed::resizeLed(double factX, double factY)
{
    double width = originalLedWidth * factX;
    double height = originalLedHeight * factY;
    if (width < 1.0) width = 1.0;
    if (height < 1.0) height = 1.0;
    setLedHeight(qRound(height));
    setLedWidth(qRound(width));
}

caLed::caLed(QWidget *parent) : ELed(parent)
{
    thisBitNr = 0;
    thisFalseColor = Qt::gray;
    thisTrueColor = Qt::red;
    thisUndefinedColor = Qt::black;
    thisColorMode = Static;
    setTrueValue("1");
    setFalseValue("0");
    return;
}

QString caLed::getPV() const
    {
       return thisPV;
    }

void caLed::setPV(QString const &newPV)
    {
        thisPV = newPV;
    }

int caLed::getBitNr() const
    {
       return thisBitNr;
    }

void caLed::setBitNr(int const newBitNr)
    {
        thisBitNr = newBitNr;
    }

void caLed::setTrueColor(QColor c)
{
    thisTrueColor = c;
}

void caLed::setUndefinedColor(QColor c)
{
    thisUndefinedColor = c;
}

void caLed::setFalseColor(QColor c)
{
    thisFalseColor = c;
    ELed::setColor(c);
}

void caLed::setState(Qt::CheckState state)
{
    if(state == Qt::Checked) {
        ELed::setColor(thisTrueColor);
    } else if(state ==  Qt::Unchecked){
        ELed::setColor(thisFalseColor);
    } else {
        ELed::setColor(thisUndefinedColor);
    }
}

void caLed::setColorMode(colMode colormode)
{
    thisColorMode = colormode;
}

void caLed::setAlarmColors(short status)
{
    switch (status) {

    case NO_ALARM:
        ELed::setColor(AL_GREEN);
        break;
    case MINOR_ALARM:
        ELed::setColor(AL_YELLOW);
        break;
    case MAJOR_ALARM:
        ELed::setColor(AL_RED);
        break;
    case INVALID_ALARM:
    case NOTCONNECTED:
        ELed::setColor(AL_WHITE);
        break;
    default:
        ELed::setColor(AL_DEFAULT);
        break;
    }
}

