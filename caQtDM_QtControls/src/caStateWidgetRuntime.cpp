#include "caStateWidgetRuntime.h"

#include <QStringList>

Qt::CheckState caStateWidgetRuntime::decodeState(const QString &trueValue, const QString &falseValue,
                                                 const knobData &data, const QString &value)
{
    const QString trueString = trueValue.trimmed();
    const QString falseString = falseValue.trimmed();
    bool trueIsNumber, falseIsNumber;
    Qt::CheckState state = Qt::Unchecked;

    if (data.edata.fieldtype == caINT || data.edata.fieldtype == caLONG) {
        const int trueNumber = trueString.toInt(&trueIsNumber);
        const int falseNumber = falseString.toInt(&falseIsNumber);
        if (trueIsNumber && trueNumber == data.edata.ivalue)
            state = Qt::Checked;
        else if (falseIsNumber && falseNumber == data.edata.ivalue)
            state = Qt::Unchecked;
        else
            state = Qt::PartiallyChecked;
    } else if (data.edata.fieldtype == caFLOAT || data.edata.fieldtype == caDOUBLE) {
        const double trueNumber = trueString.toDouble(&trueIsNumber);
        const double falseNumber = falseString.toDouble(&falseIsNumber);
        if (trueIsNumber && trueNumber == data.edata.rvalue)
            state = Qt::Checked;
        else if (falseIsNumber && falseNumber == data.edata.rvalue)
            state = Qt::Unchecked;
        else
            state = Qt::PartiallyChecked;
    } else if (data.edata.fieldtype == caENUM || data.edata.fieldtype == caSTRING ||
               data.edata.fieldtype == caCHAR) {
        const int trueNumber = trueString.toInt(&trueIsNumber);
        const int falseNumber = falseString.toInt(&falseIsNumber);
        const QStringList choices = value.split(QChar(27));
        QString selected;
        if (data.edata.ivalue < choices.count() && choices.count() > 0)
            selected = choices.at(int(data.edata.ivalue));

        state = Qt::PartiallyChecked;
        if (trueIsNumber ? trueNumber == data.edata.ivalue : trueString == selected)
            state = Qt::Checked;
        // Preserve the existing precedence when both definitions match.
        if (falseIsNumber ? falseNumber == data.edata.ivalue : falseString == selected)
            state = Qt::Unchecked;
    }
    return state;
}
