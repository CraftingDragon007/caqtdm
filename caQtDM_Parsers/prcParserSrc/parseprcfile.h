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
 *  New implementation of the PSI .prc (pep resource file) to .ui converter.
 *  Replacement for the historic ParsePepFile; original pep.tcl by
 *  Werner Portmann (PSI).
 */

#ifndef PARSEPRCFILE_H
#define PARSEPRCFILE_H

#include <QBuffer>
#include <QColor>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include "prcparserdefs.h"
#include "prctokenizer.h"

class QWidget;
class PrcUiWriter;

// all pixel metrics used by the emitters; two presets (compact = pep-like
// density, large = sizes of the historic ParsePepFile output)
struct PRCPARSER_EXPORT PrcMetrics {
    int fontSize;
    int rowHeight;
    int spacing;
    int lineEditMinWidth;
    int menuMinWidth;
    int buttonHeight;
    int spinboxMinWidth;
    int spinboxMaxWidth;
    int spinboxHeight;
    int ledFrameSize;
    int ledSize;
    int toggleMinWidth;
    int compareFontSize;

    static PrcMetrics compact();
    static PrcMetrics large();
};

enum PrcLayoutMode { PrcLayoutFromEnv, PrcLayoutCompact, PrcLayoutLarge };

class PRCPARSER_EXPORT ParsePrcFile
{
public:
    ParsePrcFile(const QString &filename, bool willPrint = false,
                 PrcLayoutMode layoutMode = PrcLayoutFromEnv);
    ~ParsePrcFile();

    QWidget *load(QWidget *parent);          // re-callable (QUiLoader on buffer)
    QByteArray uiData() const;
    QString title() const { return windowTitle; }
    bool ok() const { return parsedOk; }
    const QHash<QString, int> &unsupportedStatistics() const { return unsupported; }

private:
    enum Kind {
        KindUnknown, KindComment, KindSeparator, KindFormRead, KindSetRdbk,
        KindWheelSwitch, KindLed, KindBinary, KindText, KindEntry, KindMenu,
        KindChoice, KindSlider, KindBar, KindCompare, KindMessageButton
    };

    struct Item {
        Kind kind;
        QString channel;
        QString text;              // -text value or collected comment text
        bool textPresent;          // -text or -notext seen
        bool notext;
        PrcFormat fmt[2];
        int nfmt;
        int span;
        int comsize;
        QString comjust;           // left/center/right
        QString command, comlab;
        QString label;             // -label (messagebutton)
        QString visi;              // raw -visi condition
        QString ledstate;          // raw -ledstate list
        QString cegu, calc;
        QString printvar;          // format token of -printvar
        PrcFormat troughFormat;
        bool trough;
        bool desc;
        bool hys, confirm, compact, put;
        bool hasOPR;
        double oprMin, oprMax;
        int width;                 // -width (characters)
        int minwidth;              // -minwidth (pixels, caQtDM extension)
        int height;                // -height (separator)
        int linewidth, sepsize;
        QColor fg, bg, sepbg;
        bool hasFg, hasBg, hasSepBg;
        QStringList extra;         // leftover tokens (e.g. messagebutton value)
        int nbElem;                // occupied sub-columns

        Item() : kind(KindUnknown), textPresent(false), notext(false), nfmt(0),
                 span(1), comsize(0), trough(false), desc(false), hys(false),
                 confirm(false), compact(false), put(false), hasOPR(false),
                 oprMin(0), oprMax(0), width(0), minwidth(0), height(2),
                 linewidth(0), sepsize(0), fg(0, 0, 0), bg(200, 200, 200),
                 hasFg(false), hasBg(false), hasSepBg(false), nbElem(1) {}
    };

    struct Page {
        QString label;
        QList<QList<Item> > rows;
    };

    // parsing
    void treatFile(const QString &filename);
    bool handleDirective(const QString &line);
    void parseItemLine(const QList<PrcToken> &tokens, int lineNumber);
    Kind kindFromString(const QString &s) const;
    int computeNbElem(const Item &item) const;
    void placeItem(const Item &item);
    void warnUnsupported(const QString &what);

    // generation
    void generate();
    void writePage(PrcUiWriter &w, const Page &page);
    // minimum panel size estimate (mirrors the emitter metrics)
    void computePageMinimum(const Page &page, int &minW, int &minH) const;
    int contentMinWidth(const Item &item) const;
    int itemCellHeight(const Item &item) const;
    int textWidth(const QString &text, int pointSize) const;
    void writeRow(PrcUiWriter &w, const QList<Item> &row, int rowIndex);

    // grid-cell emitters (label column, comments, separators)
    void emitLabelCell(PrcUiWriter &w, const Item &item, int row, int col);
    void emitComment(PrcUiWriter &w, const Item &item, int row, int col, int colspan);
    void emitSeparator(PrcUiWriter &w, const Item &item, int row, int col, int colspan);
    // content emitters: widgets packed into the item content cell (pep model)
    void emitFormReadContent(PrcUiWriter &w, const Item &item);
    void emitSetRdbkContent(PrcUiWriter &w, const Item &item);
    void emitWheelSwitchContent(PrcUiWriter &w, const Item &item);
    void emitLedContent(PrcUiWriter &w, const Item &item);
    void emitBinaryContent(PrcUiWriter &w, const Item &item);
    void emitTextContent(PrcUiWriter &w, const Item &item);
    void emitEntryContent(PrcUiWriter &w, const Item &item);
    void emitMenuContent(PrcUiWriter &w, const Item &item);
    void emitChoiceContent(PrcUiWriter &w, const Item &item);
    void emitSliderContent(PrcUiWriter &w, const Item &item);
    void emitBarContent(PrcUiWriter &w, const Item &item);
    void emitCompareContent(PrcUiWriter &w, const Item &item);
    void emitMessageButtonContent(PrcUiWriter &w, const Item &item);

    // building blocks
    void writeLabelWidget(PrcUiWriter &w, const Item &item, const QString &text,
                          bool transparent, int pointSize = 0);
    void writeLineEditWidget(PrcUiWriter &w, const Item &item, const QString &pv,
                             const PrcFormat &fmt, int minWidth, bool invisible = false);
    void writeSpinboxWidget(PrcUiWriter &w, const Item &item, const QString &pv,
                            const PrcFormat &fmt);
    void writeCompareStack(PrcUiWriter &w, const QString &pv);
    void writeShellCommandWidget(PrcUiWriter &w, const QString &label, const QString &command);
    void applyFormat(PrcUiWriter &w, const PrcFormat &fmt, const QString &prefix);
    bool beginVisibilityWrapper(PrcUiWriter &w, const Item &item);
    void endVisibilityWrapper(PrcUiWriter &w, bool wrapped);
    bool visibilityToCalc(const QString &cond, QString &channel, QString &calcExpr) const;
    QString uniqueName(const QString &base);

    // data
    QList<Page> pages;
    QString windowTitle;
    QColor windowBg;
    int grid;                     // super columns (#!grid)
    int actualColumn;             // parser cursor
    bool willPrint;
    bool parsedOk;
    PrcMetrics metrics;
    QBuffer *buffer;
    QHash<QString, int> unsupported;
    QHash<QString, bool> warnedOnce;
    int nameCounter;
};

#endif // PARSEPRCFILE_H
