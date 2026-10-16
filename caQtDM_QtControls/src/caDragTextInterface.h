/*
 *  This file is part of the caQtDM Framework.
 *  Distributed under the GNU General Public License, version 3 or later.
 */

#ifndef CADRAGTEXTINTERFACE_H
#define CADRAGTEXTINTERFACE_H

#include <QString>

// Optional drag-text capability. Depth is the exact QObject-parent distance
// from the pressed child; priority preserves the legacy branch ordering.
class caDragTextInterface
{
public:
    enum BranchPriority {
        Calc = 0,
        MenuSelf, MenuParent, ChoiceSelf, ChoiceParent,
        Thermo, Slider, Clock, Meter, LinearGauge, CircularGauge,
        ByteParent, ByteControllerParent, LineEdit, MultiLineParent, Led,
        ApplyNumericSelf, ApplyNumericParent, ApplyNumericGrandparent,
        NumericSelf, NumericParent, Spinbox, ToggleButton, CameraGreatGrandparent,
        MessageButton, ImageParent, Graphics, Label, VerticalLabel, PolyLine,
        WaterfallGrandparent
    };
    virtual ~caDragTextInterface() {}
    virtual int dragTextPriority(int depth) const = 0;
    virtual QString dragTextValue() = 0;
};

#endif // CADRAGTEXTINTERFACE_H
