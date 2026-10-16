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

#ifndef caVertLabel_H
#define caVertLabel_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QPropertyAnimation>
#include <qtcontrols_global.h>
#include "fontscalingwidget.h"
#include "caLabelUpdateMixin.h"
#include "caVisibilityMonitorInterface.h"
#include "caDragTextInterface.h"

class QTCON_EXPORT caLabelVertical : public QWidget, public FontScalingWidget,
                                    public caLabelUpdateMixin, public caVisibilityMonitorInterface, public caDragTextInterface
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(Direction direction READ getDirection WRITE setDirection)
    Q_PROPERTY(Alignment alignment READ getAlignment WRITE setAlignment)
    Q_PROPERTY(QColor foreground READ getForeground WRITE setForeground)
    Q_PROPERTY(QColor background READ getBackground WRITE setBackground)
    Q_PROPERTY(QColor borderColor READ getBorderColor WRITE setBorderColor)
    Q_PROPERTY(int borderWidth READ getBorderWidth WRITE setBorderWidth)
    Q_PROPERTY(colMode colorMode READ getColorMode WRITE setColorMode)

    // this will prevent user interference
    Q_PROPERTY(QString styleSheet READ styleSheet WRITE noStyle DESIGNABLE false)

    Q_ENUMS(colMode)
    Q_ENUMS(Direction)
    Q_ENUMS(Alignment)

#include "caVisibProps.h"
#include "caVisibDefs.h"

public:
    int dragTextPriority(int depth) const override { return depth == 0 ? VerticalLabel : -1; }
    QString dragTextValue() override { return getChannelA(); }
    void noStyle(QString style) {Q_UNUSED(style);}

    enum ScaleMode { None, Height, WidthAndHeight};
    enum Alignment { Center, Left, Right};
    enum Direction {Up, Down};

    caLabelVertical(QWidget *parent = 0);
    bool rotateText(float degrees);

    void setText(const QString &text);
    QString text() const {return thisText;}

    void setDirection(const Direction &direction);
    Direction getDirection() const {return thisDirection;}

    void setAlignment(const Alignment &alignment);
    Alignment getAlignment() const {return thisAlignment;}

    QSize calculateTextSpace() override;

    void setFontScaleMode(ScaleMode m) { FontScalingWidget::setScaleMode((int) m); }
    ScaleMode fontScaleMode() { return (ScaleMode) FontScalingWidget::scaleMode();  }

    QColor getForeground() const {return thisForeColor;}
    void setForeground(QColor c);

    QColor getBackground() const {return thisBackColor;}
    void setBackground(QColor c);

    QColor getBorderColor() const {return thisBorderColor;}
    void setBorderColor(QColor c);

    void setBorderWidth(int width) {thisBorderWidth = width;  update();}
    int getBorderWidth() const {return thisBorderWidth;}

    enum colMode {Static, Alarm};
    colMode getColorMode() const { return thisColorMode; }

    void setColors(QColor bg, QColor fg);

    void setAlarmColors(short status);

    void setColorMode(colMode colormode) {thisColorMode = colormode;
                                          setBackground(thisBackColor);
                                          setForeground(thisForeColor);}
public slots:
    void animation(QRect p) {
#include "animationcode.h"
    }

    void hideObject(bool hideit) {
#include "hideobjectcode.h"
    }

protected:
    virtual bool event(QEvent *);
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

private:
    int labelObjectType() const override { return caLabelVertical_Widget; }
    QString labelText() const override { return text(); }
    void setLabelText(const QString &value) override { setText(value); }

    Mode visibilityMode() const override { return static_cast<Mode>(getVisibility()); }
    bool skipVisibilityMonitors() const override
    {
        return getVisibility() == StaticV && getColorMode() == Static;
    }
    QString visibilityChannel(int index) const override
    {
        switch (index) {
        case 0: return getChannelA();
        case 1: return getChannelB();
        case 2: return getChannelC();
        case 3: return getChannelD();
        default: return QString();
        }
    }
    void setVisibilityChannel(int index, const QString &pv) override
    {
        switch (index) {
        case 0: setChannelA(pv); break;
        case 1: setChannelB(pv); break;
        case 2: setChannelC(pv); break;
        case 3: setChannelD(pv); break;
        }
    }
    QString visibilityExpression() const override { return getVisibilityCalc(); }
    void setVisibilityExpression(const QString &value) override { setVisibilityCalc(value); }

    QWidget *labelWidget() override { return this; }
    ColorBehavior colorBehavior() const override
    {
        return thisColorMode == Alarm ? AlarmColors : StaticColors;
    }
    void restoreLabelColors(const QColor &foreground, const QColor &background) override
    {
        setForeground(foreground);
        setBackground(background);
    }
    void setLabelAlarmColors(short status) override { setAlarmColors(status); }

    float rotation;
    QString thisText;
    QColor thisForeColor;
    QColor thisBackColor;
    QColor thisBorderColor;
    int thisBorderWidth;
    colMode thisColorMode;
    Direction thisDirection;
    Alignment thisAlignment;

protected:
    void paintEvent(QPaintEvent *);

};
#endif
