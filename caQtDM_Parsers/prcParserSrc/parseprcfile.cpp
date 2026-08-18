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

#include "parseprcfile.h"
#include "prcuiwriter.h"

#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QTextStream>
#include <QRegularExpression>
#include <cstring>
#include <QtUiTools/QUiLoader>

Q_LOGGING_CATEGORY(parsePrcFileLog, "caqtdm.parsers.prc")

static const char *prcFontFamily = "Lucida Sans Typewriter";

PrcMetrics PrcMetrics::compact()
{
    PrcMetrics m;
    m.fontSize = 10;
    m.rowHeight = 18;
    m.spacing = 2;
    m.lineEditMinWidth = 70;
    m.menuMinWidth = 90;
    m.buttonHeight = 20;
    m.spinboxMinWidth = 80;
    m.spinboxMaxWidth = 120;
    m.spinboxHeight = 20;
    m.ledFrameSize = 16;
    m.ledSize = 12;
    m.toggleMinWidth = 55;
    m.compareFontSize = 12;
    return m;
}

PrcMetrics PrcMetrics::large()
{
    // sizes of the historic ParsePepFile output
    PrcMetrics m;
    m.fontSize = 12;
    m.rowHeight = 20;
    m.spacing = 5;
    m.lineEditMinWidth = 100;
    m.menuMinWidth = 120;
    m.buttonHeight = 24;
    m.spinboxMinWidth = 100;
    m.spinboxMaxWidth = 150;
    m.spinboxHeight = 25;
    m.ledFrameSize = 24;
    m.ledSize = 18;
    m.toggleMinWidth = 65;
    m.compareFontSize = 16;
    return m;
}

static PrcMetrics selectMetrics(PrcLayoutMode mode)
{
    if(mode == PrcLayoutCompact) return PrcMetrics::compact();
    if(mode == PrcLayoutLarge) return PrcMetrics::large();
    static const bool large = (qgetenv("CAQTDM_PRC_LARGE_LAYOUT").trimmed() == "1");
    return large ? PrcMetrics::large() : PrcMetrics::compact();
}

ParsePrcFile::ParsePrcFile(const QString &filename, bool willprint, PrcLayoutMode layoutMode)
    : windowBg(218, 218, 218), grid(1), actualColumn(0), willPrint(willprint),
      parsedOk(false), metrics(selectMetrics(layoutMode)), buffer(new QBuffer()),
      nameCounter(0)
{
    pages.append(Page());
    treatFile(filename);
    if(parsedOk) generate();
}

ParsePrcFile::~ParsePrcFile()
{
    delete buffer;
}

QByteArray ParsePrcFile::uiData() const
{
    return buffer->data();
}

QWidget *ParsePrcFile::load(QWidget *parent)
{
    QUiLoader loader;
    buffer->open(QIODevice::ReadOnly);
    buffer->seek(0);
    QWidget *widget = loader.load(buffer, parent);
    buffer->close();
    return widget;
}

QString ParsePrcFile::uniqueName(const QString &base)
{
    return base + QString("_%1").arg(++nameCounter);
}

void ParsePrcFile::warnUnsupported(const QString &what)
{
    unsupported[what]++;
    if(!warnedOnce.contains(what)) {
        warnedOnce[what] = true;
        qCWarning(parsePrcFileLog) << what << "is not supported";
    }
}

//===========================================================================
// parsing
//===========================================================================

void ParsePrcFile::treatFile(const QString &filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(parsePrcFileLog) << "cannot open" << filename;
        return;
    }
    parsedOk = true;

    QTextStream in(&file);
    int lineNumber = 0;
    while(!in.atEnd()) {
        QString line = in.readLine();
        lineNumber++;
        line.replace(QLatin1Char('\t'), QLatin1Char(' '));
        const QString trimmed = line.trimmed();
        if(trimmed.isEmpty()) continue;

        if(trimmed.startsWith("#!")) {
            handleDirective(trimmed);
            continue;
        }
        if(trimmed.startsWith(QLatin1Char('#'))) continue;   // plain comment / CVS line

        const QList<PrcToken> tokens = PrcTokenizer::tokenize(trimmed);
        if(tokens.count() < 2) {
            qCWarning(parsePrcFileLog) << "line" << lineNumber << "ignored (needs channel and type):" << trimmed;
            continue;
        }
        // skip the conventional header line "channel name  type  args"
        if(tokens.at(0).text == "channel" && tokens.at(1).text == "name") continue;

        parseItemLine(tokens, lineNumber);
    }
    file.close();
}

bool ParsePrcFile::handleDirective(const QString &line)
{
    const QList<PrcToken> tokens = PrcTokenizer::tokenize(line);
    if(tokens.isEmpty()) return false;
    const QString directive = tokens.at(0).text;
    const QString arg = tokens.count() > 1 ? tokens.at(1).text : QString();

    if(directive == "#!grid") {
        const int g = arg.toInt();
        if(g > 0) grid = g;
        qCDebug(parsePrcFileLog) << "grid =" << grid;
    } else if(directive == "#!title") {
        windowTitle = arg;
    } else if(directive == "#!qtbg") {
        const QColor c(arg);
        if(c.isValid()) windowBg = c;
        else qCWarning(parsePrcFileLog) << "invalid color for #!qtbg:" << arg;
    } else if(directive == "#!tab") {
        // first #!tab renames the implicit first page if it is still empty
        if(pages.count() == 1 && pages.first().rows.isEmpty() && pages.first().label.isEmpty()) {
            pages.first().label = arg;
        } else {
            pages.append(Page());
            pages.last().label = arg;
        }
        actualColumn = 0;
    } else if(directive == "#!setup" || directive == "#!printvar") {
        qCDebug(parsePrcFileLog) << directive << "ignored";
    } else if(directive == "#!newline" || directive == "#!bg" || directive == "#!ifmacro") {
        warnUnsupported(directive);
    } else {
        qCDebug(parsePrcFileLog) << "unknown directive" << directive << "ignored";
    }
    return true;
}

ParsePrcFile::Kind ParsePrcFile::kindFromString(const QString &s) const
{
    if(s == "comment") return KindComment;
    if(s == "separator") return KindSeparator;
    if(s == "formread") return KindFormRead;
    if(s == "setrdbk" || s == "mactrl") return KindSetRdbk;
    if(s == "wheelswitch") return KindWheelSwitch;
    if(s == "led") return KindLed;
    if(s == "binary") return KindBinary;
    if(s == "text") return KindText;
    if(s == "entry") return KindEntry;
    if(s == "menubutton") return KindMenu;
    if(s == "choicebutton") return KindChoice;
    if(s == "slider") return KindSlider;
    if(s == "bar") return KindBar;
    if(s == "compare") return KindCompare;
    if(s == "messagebutton") return KindMessageButton;
    return KindUnknown;
}

static bool kindTakesFormats(int kind)
{
    // KindFormRead, KindSetRdbk, KindWheelSwitch, KindText, KindEntry,
    // KindSlider, KindBar (numeric ordinals of the private enum)
    return kind == 3 || kind == 4 || kind == 5 || kind == 8 || kind == 9
        || kind == 12 || kind == 13;
}

void ParsePrcFile::parseItemLine(const QList<PrcToken> &tokens, int lineNumber)
{
    Item item;
    item.channel = tokens.at(0).text;
    item.kind = kindFromString(tokens.at(1).text.toLower());

    if(item.kind == KindUnknown) {
        warnUnsupported(QString("widget type '%1'").arg(tokens.at(1).text));
        qCWarning(parsePrcFileLog) << "line" << lineNumber << ": unknown widget type" << tokens.at(1).text;
        // keep the grid cell occupied like the old parser did (empty cell)
        placeItem(item);
        return;
    }

    const bool isCommentLike = (item.kind == KindComment || item.kind == KindSeparator);
    QStringList textParts;

    int i = 2;
    const int n = tokens.count();
    // fetch the value of an option; empty (and warning) when missing
    auto optArg = [&](const QString &name) -> QString {
        if(i + 1 < n) return tokens.at(++i).text;
        qCWarning(parsePrcFileLog) << "line" << lineNumber << ": option" << name << "without value";
        return QString();
    };

    for(; i < n; i++) {
        const PrcToken &tok = tokens.at(i);
        const QString &t = tok.text;

        if(!tok.quoted && t.startsWith(QLatin1Char('-')) && t.size() > 1 && !t.at(1).isDigit()) {
            if(t == "-span")            { item.span = qMax(1, optArg("-span").toInt()); }
            else if(t == "-comsize")    { item.comsize = optArg("-comsize").toInt(); }
            else if(t == "-comjust")    { item.comjust = optArg("-comjust").toLower(); }
            else if(t == "-comfg" || t == "-fg") {
                const QColor c(optArg(t));
                if(c.isValid()) { item.fg = c; item.hasFg = true; }
            }
            else if(t == "-bg") {
                const QColor c(optArg("-bg"));
                if(c.isValid()) { item.bg = c; item.hasBg = true; }
            }
            else if(t == "-sepbg") {
                const QColor c(optArg("-sepbg"));
                if(c.isValid()) { item.sepbg = c; item.hasSepBg = true; }
            }
            else if(t == "-height")     { item.height = qMax(1, optArg("-height").toInt()); }
            else if(t == "-linewidth")  { item.linewidth = optArg("-linewidth").toInt(); }
            else if(t == "-sepsize")    { item.sepsize = optArg("-sepsize").toInt(); }
            else if(t == "-minwidth")   { item.minwidth = optArg("-minwidth").toInt(); }
            else if(t == "-width")      { item.width = optArg("-width").toInt(); }
            else if(t == "-text")       { item.text = optArg("-text"); item.textPresent = true; }
            else if(t == "-notext")     { item.text.clear(); item.textPresent = true; item.notext = true; }
            else if(t == "-desc")       { item.desc = true; }
            else if(t == "-label")      { item.label = optArg("-label"); }
            else if(t == "-command")    { item.command = optArg("-command"); }
            else if(t == "-comlab")     { item.comlab = optArg("-comlab"); }
            else if(t == "-visi")       { item.visi = optArg("-visi"); }
            else if(t == "-ledstate")   { item.ledstate = optArg("-ledstate"); }
            else if(t == "-cegu")       { item.cegu = optArg("-cegu"); }
            else if(t == "-calc")       { item.calc = optArg("-calc"); }
            else if(t == "-printvar")   { item.printvar = optArg("-printvar"); }
            else if(t == "-trough") {
                item.trough = true;
                if(i + 1 < n && !tokens.at(i + 1).quoted) {
                    const PrcFormat f = PrcTokenizer::parseFormat(tokens.at(i + 1).text);
                    if(f.valid) { item.troughFormat = f; i++; }
                }
            }
            else if(t == "-OPR") {
                const QString mi = optArg("-OPR");
                const QString ma = optArg("-OPR");
                bool ok1 = false, ok2 = false;
                const double vmin = mi.toDouble(&ok1);
                const double vmax = ma.toDouble(&ok2);
                if(ok1 && ok2) { item.oprMin = vmin; item.oprMax = vmax; item.hasOPR = true; }
                else qCWarning(parsePrcFileLog) << "line" << lineNumber << ": -OPR needs two numeric values";
            }
            else if(t == "-hys")        { item.hys = true; }
            else if(t == "-confirm")    { item.confirm = true; }
            else if(t == "-compact")    { item.compact = true; }
            else if(t == "-put")        { item.put = true; }
            else if(t == "-useitems")   { (void) optArg("-useitems"); warnUnsupported("-useitems"); }
            else if(isCommentLike)      { textParts.append(t); }   // e.g. old "-7.2s" style tokens
            else                        { warnUnsupported(t); }
        } else {
            // not an option: format candidate or free text
            if(!tok.quoted && !isCommentLike && kindTakesFormats(item.kind) && item.nfmt < 2) {
                const PrcFormat f = PrcTokenizer::parseFormat(t);
                if(f.valid) { item.fmt[item.nfmt++] = f; continue; }
            }
            if(isCommentLike) textParts.append(t);
            else item.extra.append(t);
        }
    }

    if(isCommentLike) {
        // drop the repeated type word ("comment comment …" / "separator separator …")
        item.text = textParts.join(" ");
        if(item.text.startsWith(item.channel)) item.text = item.text.mid(item.channel.size()).trimmed();
    }

    if(item.confirm) {
        if(!warnedOnce.contains("-confirm")) {
            warnedOnce["-confirm"] = true;
            qCInfo(parsePrcFileLog) << "-confirm has no equivalent in caQtDM widgets and is ignored";
        }
        unsupported["-confirm"]++;
    }

    item.nbElem = computeNbElem(item);
    placeItem(item);
}

int ParsePrcFile::computeNbElem(const Item &item) const
{
    const int label = item.notext ? 0 : 1;
    const int command = item.command.isEmpty() ? 0 : 1;
    switch(item.kind) {
    case KindComment: {
        int nb = 1;
        if(!item.command.isEmpty() && !item.text.isEmpty()) nb++;
        return nb;
    }
    case KindSeparator:      return 1;
    case KindFormRead:       return label + 1 + (item.cegu.isEmpty() ? 0 : 1) + command;
    case KindSetRdbk:        return label + 5;   // spinbox, compare, readback, onoff, psmode
    case KindWheelSwitch:    return label + 1 + command;
    case KindLed:            return label + 1 + command;
    case KindBinary:         return label + (item.compact ? 0 : 1) + 1 + command;
    case KindText:           return 4;
    case KindEntry:          return label + 1;
    case KindMenu:           return label + 1 + command;
    case KindChoice:         return label + 1 + command;
    case KindSlider:         return label + 1 + (item.trough ? 1 : 0) + command;
    case KindBar:            return label + 2 + (item.cegu.isEmpty() ? 0 : 1) + command;
    case KindCompare:        return label + 1;
    case KindMessageButton:  return label + 1 + command;
    default:                 return 1;
    }
}

void ParsePrcFile::placeItem(const Item &item)
{
    Page &page = pages.last();
    if(actualColumn == 0 || page.rows.isEmpty()) page.rows.append(QList<Item>());
    page.rows.last().append(item);

    actualColumn += item.span;
    if(actualColumn > grid - 1) actualColumn = 0;
}

//===========================================================================
// generation
//===========================================================================

int ParsePrcFile::textWidth(const QString &text, int pointSize) const
{
    QFont font(prcFontFamily, pointSize);
    QFontMetrics fm(font);
    return fm.boundingRect(text).width();
}

// minimum width of the packed content cell of an item (mirrors the emitters)
int ParsePrcFile::contentMinWidth(const Item &item) const
{
    QList<int> w;
    const int fs = item.comsize > 0 ? item.comsize : metrics.fontSize;
    const int lew = item.minwidth > 0 ? item.minwidth : metrics.lineEditMinWidth;

    switch(item.kind) {
    case KindFormRead:
        w << lew;
        if(!item.cegu.isEmpty()) w << textWidth(item.cegu, fs);
        break;
    case KindSetRdbk:
        w << metrics.spinboxMinWidth << 20 << lew << metrics.menuMinWidth << lew;
        break;
    case KindWheelSwitch:    w << metrics.spinboxMinWidth; break;
    case KindLed:            w << metrics.ledFrameSize; break;
    case KindBinary:
        if(!item.compact) w << lew;
        w << metrics.toggleMinWidth;
        break;
    case KindText:           w << lew << textWidth("new value", fs) << metrics.lineEditMinWidth; break;
    case KindEntry:          w << lew; break;
    case KindMenu:
    case KindChoice:         w << metrics.menuMinWidth; break;
    case KindSlider:
        w << metrics.spinboxMaxWidth;
        if(item.trough) w << lew;
        break;
    case KindBar:
        w << metrics.spinboxMaxWidth << lew;
        if(!item.cegu.isEmpty()) w << textWidth(item.cegu, fs);
        break;
    case KindCompare:        w << 20; break;
    case KindMessageButton: {
        const QString label = item.label.isEmpty() ? QString("?") : item.label;
        w << 10 * (label.size() + 1);
        break; }
    default: break;
    }
    if(!item.command.isEmpty()) w << 10 * (item.comlab.size() + 1);

    int sum = 0;
    for(int i = 0; i < w.count(); i++) sum += w.at(i);
    if(w.count() > 1) sum += metrics.spacing * (w.count() - 1);
    return sum;
}

int ParsePrcFile::itemCellHeight(const Item &item) const
{
    switch(item.kind) {
    case KindSetRdbk:
    case KindCompare:        return 2 * 20 + metrics.spacing;   // compare stack
    case KindWheelSwitch:    return metrics.spinboxHeight;
    case KindMenu:
    case KindChoice:
    case KindBinary:
    case KindSlider:
    case KindBar:
    case KindMessageButton:  return metrics.buttonHeight;
    case KindSeparator:      return qMax(2, item.height);
    default:                 return metrics.rowHeight;
    }
}

void ParsePrcFile::computePageMinimum(const Page &page, int &minW, int &minH) const
{
    // pep layout: per super column one label column and one content column
    QList<int> labelW, contentW;
    for(int i = 0; i < grid; i++) { labelW.append(0); contentW.append(0); }
    int wideMin = 0;
    minH = 0;

    for(int r = 0; r < page.rows.count(); r++) {
        const QList<Item> &row = page.rows.at(r);
        int rowHeight = 0;
        int sc = 0;
        for(int i = 0; i < row.count() && sc < grid; i++) {
            const Item &item = row.at(i);
            const int spanEnd = qMin(sc + item.span, grid);
            const int fs = item.comsize > 0 ? item.comsize : metrics.fontSize;

            if(item.kind == KindComment || item.kind == KindSeparator) {
                const int tw = textWidth(item.text, fs) + (item.command.isEmpty() ? 0 : 10 * (item.comlab.size() + 1));
                if(tw > wideMin) wideMin = tw;
            } else if(item.kind != KindUnknown) {
                if(!item.notext) {
                    const int lw = item.desc ? metrics.lineEditMinWidth
                                             : textWidth(item.textPresent ? item.text : item.channel, fs);
                    if(lw > labelW[sc]) labelW[sc] = lw;
                }
                const int cw = contentMinWidth(item) / (spanEnd - sc);
                for(int k = sc; k < spanEnd; k++)
                    if(cw > contentW[k]) contentW[k] = cw;
            }
            const int hh = itemCellHeight(item);
            if(hh > rowHeight) rowHeight = hh;
            sc += item.span;
        }
        minH += rowHeight + metrics.spacing;
    }
    minW = metrics.spacing * (2 * grid - 1);
    for(int i = 0; i < grid; i++) minW += labelW.at(i) + contentW.at(i);
    if(wideMin > minW) minW = wideMin;
}

void ParsePrcFile::generate()
{
    buffer->open(QIODevice::WriteOnly | QIODevice::Truncate);
    PrcUiWriter w(buffer);

    w.startDocument();
    w.startWidget("QMainWindow", "MainWindow");
    if(!windowTitle.isEmpty()) w.stringProperty("windowTitle", windowTitle);
    w.stringProperty("styleSheet",
                     QString("QWidget#centralWidget {background: %1; }\n"
                             "caLineEdit {border-radius: 1px;background: white; color: black;}\n")
                     .arg(windowBg.name()), true);
    w.startWidget("QWidget", "centralWidget");

    // minimum panel size so the window cannot be squeezed together
    const bool useTabs = pages.count() > 1 || !pages.first().label.isEmpty();
    int minW = 0, minH = 0;
    for(int p = 0; p < pages.count(); p++) {
        int pw = 0, ph = 0;
        computePageMinimum(pages.at(p), pw, ph);
        if(pw > minW) minW = pw;
        if(useTabs) { if(ph > minH) minH = ph; }
        else minH += ph;
    }
    if(useTabs) { minW += 12; minH += 44; }   // tab frame and tab bar
    minW += 24; minH += 24;                   // layout margins
    w.sizeProperty("minimumSize", minW, minH);

    w.startLayout("QGridLayout", "outerLayout");
    w.startItem(0, 0);

    if(useTabs) {
        w.startWidget("QTabWidget", "prcTabWidget");
        w.numberProperty("currentIndex", 0);
        for(int p = 0; p < pages.count(); p++) {
            w.startWidget("QWidget", uniqueName("tab"));
            w.raw().writeStartElement("attribute");
            w.raw().writeAttribute("name", "title");
            w.raw().writeTextElement("string", pages.at(p).label.isEmpty()
                                     ? QString("Tab %1").arg(p + 1) : pages.at(p).label);
            w.raw().writeEndElement();
            writePage(w, pages.at(p));
            w.endWidget();
        }
        w.endWidget();
    } else {
        // plain page: nested grid like the old parser
        writePage(w, pages.first());
    }

    w.endItem();
    w.endLayout();
    w.endWidget();   // centralWidget
    w.endWidget();   // MainWindow
    w.endDocument();

    buffer->close();
}

void ParsePrcFile::writePage(PrcUiWriter &w, const Page &page)
{
    w.startLayout("QGridLayout", uniqueName("gridLayout"));
    w.numberProperty("spacing", metrics.spacing);
    for(int r = 0; r < page.rows.count(); r++)
        writeRow(w, page.rows.at(r), r);
    w.endLayout();
}

void ParsePrcFile::writeRow(PrcUiWriter &w, const QList<Item> &row, int rowIndex)
{
    // pep layout model: every item uses two grid columns, a label column and
    // one content cell in which the widgets of the item are packed (this is
    // what kept the original pep panels narrow)
    int sc = 0;
    for(int i = 0; i < row.count(); i++) {
        const Item &item = row.at(i);
        if(sc >= grid) break;
        const int spanEnd = qMin(sc + item.span, grid);
        const int fullSpan = 2 * (spanEnd - sc);

        if(item.kind == KindComment) {
            emitComment(w, item, rowIndex, 2 * sc, fullSpan);
            sc += item.span;
            continue;
        }
        if(item.kind == KindSeparator) {
            emitSeparator(w, item, rowIndex, 2 * sc, fullSpan);
            sc += item.span;
            continue;
        }
        if(item.kind == KindUnknown) { sc += item.span; continue; }

        emitLabelCell(w, item, rowIndex, 2 * sc);

        w.startItem(rowIndex, 2 * sc + 1, fullSpan - 1);
        const bool wrapped = beginVisibilityWrapper(w, item);
        w.startLayout("QHBoxLayout", uniqueName("itemlayout"));
        w.numberProperty("spacing", metrics.spacing);

        switch(item.kind) {
        case KindFormRead:      emitFormReadContent(w, item); break;
        case KindSetRdbk:       emitSetRdbkContent(w, item); break;
        case KindWheelSwitch:   emitWheelSwitchContent(w, item); break;
        case KindLed:           emitLedContent(w, item); break;
        case KindBinary:        emitBinaryContent(w, item); break;
        case KindText:          emitTextContent(w, item); break;
        case KindEntry:         emitEntryContent(w, item); break;
        case KindMenu:          emitMenuContent(w, item); break;
        case KindChoice:        emitChoiceContent(w, item); break;
        case KindSlider:        emitSliderContent(w, item); break;
        case KindBar:           emitBarContent(w, item); break;
        case KindCompare:       emitCompareContent(w, item); break;
        case KindMessageButton: emitMessageButtonContent(w, item); break;
        default: break;
        }
        // related display button (pep addRelDis)
        if(!item.command.isEmpty()) {
            w.startItem();
            writeShellCommandWidget(w, item.comlab, item.command);
            w.endItem();
        }
        w.endLayout();
        endVisibilityWrapper(w, wrapped);
        w.endItem();
        sc += item.span;
    }
}

//===========================================================================
// visibility support
//===========================================================================

bool ParsePrcFile::visibilityToCalc(const QString &cond, QString &channel, QString &calcExpr) const
{
    static const struct { const char *op; const char *calcOp; } ops[] = {
        {"==", "="}, {"!=", "#"}, {">=", ">="}, {"<=", "<="}, {">", ">"}, {"<", "<"}
    };
    for(size_t k = 0; k < sizeof(ops) / sizeof(ops[0]); k++) {
        const int pos = cond.indexOf(ops[k].op);
        if(pos > 0) {
            channel = cond.left(pos).trimmed();
            const QString value = cond.mid(pos + int(strlen(ops[k].op))).trimmed();
            if(channel.isEmpty() || value.isEmpty()) return false;
            calcExpr = QString("A%1%2").arg(ops[k].calcOp).arg(value);
            return true;
        }
    }
    return false;
}

bool ParsePrcFile::beginVisibilityWrapper(PrcUiWriter &w, const Item &item)
{
    if(item.visi.isEmpty()) return false;
    QString channel, calcExpr;
    if(!visibilityToCalc(item.visi, channel, calcExpr)) {
        warnUnsupported(QString("-visi '%1'").arg(item.visi));
        return false;
    }
    w.startWidget("caFrame", uniqueName("visiframe"));
    w.enumProperty("visibility", "caFrame::Calc");
    w.stringProperty("visibilityCalc", calcExpr);
    w.stringProperty("channel", channel);
    w.startLayout("QHBoxLayout", uniqueName("visilayout"));
    w.numberProperty("spacing", 0);
    w.raw().writeStartElement("property");
    w.raw().writeAttribute("name", "margin");
    w.raw().writeTextElement("number", "0");
    w.raw().writeEndElement();
    w.startItem();
    return true;
}

void ParsePrcFile::endVisibilityWrapper(PrcUiWriter &w, bool wrapped)
{
    if(!wrapped) return;
    w.endItem();
    w.endLayout();
    w.endWidget();
}

//===========================================================================
// building blocks
//===========================================================================

void ParsePrcFile::writeLabelWidget(PrcUiWriter &w, const Item &item, const QString &text,
                                    bool transparent, int pointSize)
{
    const int fs = pointSize > 0 ? pointSize : (item.comsize > 0 ? item.comsize : metrics.fontSize);
    w.startWidget("caLabel", uniqueName("calabel"));

    QFont font(prcFontFamily, fs);
    QFontMetrics fm(font);
    const int width = fm.boundingRect(text).width();
    w.sizeProperty("minimumSize", width, metrics.rowHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.rowHeight);

    w.stringProperty("text", text);
    QString align = "Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter";
    if(item.comjust == "center") align = "Qt::AlignCenter";
    else if(item.comjust == "right") align = "Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter";
    w.setProperty("alignment", align);

    if(transparent) w.colorProperty("background", QColor(200, 200, 200), 0);
    else w.colorProperty("background", item.bg, 255);
    w.colorProperty("foreground", item.fg, 255);
    w.fontProperty(prcFontFamily, fs);
    w.enumProperty("fontScaleMode", "None");
    w.endWidget();
}

void ParsePrcFile::applyFormat(PrcUiWriter &w, const PrcFormat &fmt, const QString &prefix)
{
    if(!fmt.valid) return;
    const QChar c = fmt.conv;
    if(c == QLatin1Char('f') || c == QLatin1Char('d')) w.enumProperty("formatType", prefix + "::decimal");
    else if(c == QLatin1Char('e') || c == QLatin1Char('g')) w.enumProperty("formatType", prefix + "::exponential");
    else if(c == QLatin1Char('x')) w.enumProperty("formatType", prefix + "::hexadecimal");
    else if(c == QLatin1Char('o')) w.enumProperty("formatType", prefix + "::octal");
    else if(c == QLatin1Char('s')) w.enumProperty("formatType", prefix + "::string");
    if(fmt.precision >= 0 && c != QLatin1Char('s')) {
        w.enumProperty("precisionMode", prefix + "::User");
        w.numberProperty("precision", fmt.precision);
    }
}

void ParsePrcFile::writeLineEditWidget(PrcUiWriter &w, const Item &item, const QString &pv,
                                       const PrcFormat &fmt, int minWidth, bool invisible)
{
    w.startWidget("caLineEdit", uniqueName("calineedit"));
    applyFormat(w, fmt, "caLineEdit");
    const int mw = item.minwidth > 0 ? item.minwidth : minWidth;
    w.sizeProperty("minimumSize", mw, metrics.rowHeight);
    w.setProperty("alignment", "Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter");
    w.stringProperty("channel", pv);
    if(invisible) {
        w.colorProperty("background", QColor(255, 255, 255), 0);
        w.colorProperty("foreground", QColor(255, 255, 255), 0);
    } else if(!willPrint) {
        w.enumProperty("colorMode", "caLineEdit::Alarm_Default");
        w.enumProperty("alarmHandling", "caLineEdit::onBackground");
    } else {
        w.colorProperty("background", QColor(255, 255, 255), 0);
    }
    w.boolProperty("unitsEnabled", item.cegu.isEmpty());
    const int fs = item.comsize > 0 ? item.comsize : metrics.fontSize;
    w.fontProperty(prcFontFamily, fs);
    w.endWidget();
}

void ParsePrcFile::writeSpinboxWidget(PrcUiWriter &w, const Item &item, const QString &pv,
                                      const PrcFormat &fmt)
{
    int totalDigits = 8, decimalDigits = 3;
    if(fmt.valid) {
        if(fmt.width >= 0) totalDigits = fmt.width;
        if(fmt.precision >= 0) decimalDigits = fmt.precision;
    }
    const int integerDigits = totalDigits - decimalDigits - 2;

    w.startWidget("caSpinbox", uniqueName("caspinbox"));
    w.stringProperty("channel", pv);
    w.colorProperty("background", QColor(150, 245, 120), 255);
    w.sizeProperty("minimumSize", metrics.spinboxMinWidth, metrics.spinboxHeight);
    w.sizeProperty("maximumSize", metrics.spinboxMaxWidth, metrics.spinboxHeight);
    w.numberProperty("integerDigits", integerDigits);
    w.numberProperty("decimalDigits", decimalDigits);
    w.boolProperty("fixedFormat", true);
    if(item.hasOPR) {
        w.enumProperty("limitsMode", "caSpinbox::User");
        w.raw().writeStartElement("property");
        w.raw().writeAttribute("name", "minValue");
        w.raw().writeTextElement("double", QString::number(item.oprMin));
        w.raw().writeEndElement();
        w.raw().writeStartElement("property");
        w.raw().writeAttribute("name", "maxValue");
        w.raw().writeTextElement("double", QString::number(item.oprMax));
        w.raw().writeEndElement();
    }
    w.endWidget();
}

void ParsePrcFile::writeCompareStack(PrcUiWriter &w, const QString &pv)
{
    w.startLayout("QVBoxLayout", uniqueName("comparelayout"));
    w.startItem();
    w.startWidget("caLabel", uniqueName("calabel"));
    w.sizeProperty("minimumSize", 20, 20);
    w.sizeProperty("maximumSize", 20, 20);
    w.stringProperty("text", "<html><head/><body>=&nbsp;</body></html>");
    w.setProperty("alignment", "Qt::AlignLeading|Qt::AlignCenter|Qt::AlignVCenter");
    w.colorProperty("background", QColor(200, 200, 200), 0);
    w.colorProperty("foreground", QColor(0, 0, 0), 255);
    w.fontProperty(prcFontFamily, metrics.compareFontSize);
    w.enumProperty("colorMode", "caLabel::Alarm");
    w.enumProperty("fontScaleMode", "None");
    w.enumProperty("visibility", "caLabel::Calc");
    w.stringProperty("visibilityCalc", "A=0");
    w.stringProperty("channel", pv);
    w.endWidget();
    w.endItem();
    w.startItem();
    w.startWidget("caLabel", uniqueName("calabel"));
    w.sizeProperty("minimumSize", 20, 20);
    w.sizeProperty("maximumSize", 20, 20);
    w.stringProperty("text", "<html><head/><body>&ne;&nbsp;</body></html>");
    w.setProperty("alignment", "Qt::AlignLeading|Qt::AlignCenter|Qt::AlignVCenter");
    w.colorProperty("background", QColor(200, 200, 200), 0);
    w.colorProperty("foreground", QColor(0, 0, 0), 255);
    w.fontProperty(prcFontFamily, metrics.compareFontSize);
    w.enumProperty("colorMode", "caLabel::Alarm");
    w.enumProperty("fontScaleMode", "None");
    w.enumProperty("visibility", "caLabel::Calc");
    w.stringProperty("visibilityCalc", "A>0");
    w.stringProperty("channel", pv);
    w.endWidget();
    w.endItem();
    w.endLayout();
}

void ParsePrcFile::writeShellCommandWidget(PrcUiWriter &w, const QString &label, const QString &command)
{
    w.startWidget("caShellCommand", uniqueName("cashellcommand"));
    w.stringProperty("label", label);
    w.stringProperty("labels", "");
    w.stringProperty("files", "");
    w.stringProperty("args", QString(command).replace("\"", ""));
    w.sizeProperty("minimumSize", 10 * (label.size() + 1), metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    w.endWidget();
}

//===========================================================================
// cell emitters
//===========================================================================

void ParsePrcFile::emitLabelCell(PrcUiWriter &w, const Item &item, int row, int col)
{
    if(item.notext) return;
    w.startItem(row, col);
    if(item.desc) {
        // row label taken from the record description field
        w.startWidget("caLineEdit", uniqueName("calineedit"));
        w.sizeProperty("minimumSize", metrics.lineEditMinWidth, metrics.rowHeight);
        w.setProperty("alignment", "Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter");
        w.stringProperty("channel", item.channel + ".DESC");
        w.enumProperty("colorMode", "caLineEdit::Static");
        w.colorProperty("background", QColor(200, 200, 200), 0);
        w.colorProperty("foreground", item.fg, 255);
        w.boolProperty("unitsEnabled", false);
        w.boolProperty("framePresent", false);
        w.fontProperty(prcFontFamily, item.comsize > 0 ? item.comsize : metrics.fontSize);
        w.endWidget();
    } else {
        writeLabelWidget(w, item, item.textPresent ? item.text : item.channel, true);
    }
    w.endItem();
}

void ParsePrcFile::emitComment(PrcUiWriter &w, const Item &item, int row, int col, int colspan)
{
    if(!item.command.isEmpty()) {
        int c = col;
        if(!item.text.isEmpty()) {
            w.startItem(row, c, colspan > 1 ? colspan - 1 : 1);
            writeLabelWidget(w, item, item.text, false);
            w.endItem();
            c += colspan > 1 ? colspan - 1 : 1;
        }
        w.startItem(row, c);
        writeShellCommandWidget(w, item.comlab, item.command);
        w.endItem();
        return;
    }
    w.startItem(row, col, colspan);
    const bool wrapped = beginVisibilityWrapper(w, item);
    if(item.text.isEmpty())
        writeLabelWidget(w, item, "<html><head/><body>&nbsp;&nbsp;</body></html>", false);
    else
        writeLabelWidget(w, item, item.text, false);
    endVisibilityWrapper(w, wrapped);
    w.endItem();
}

void ParsePrcFile::emitSeparator(PrcUiWriter &w, const Item &item, int row, int col, int colspan)
{
    int thickness = item.height;
    if(item.linewidth > 0) thickness = qMax(item.linewidth, item.height / 2);
    if(item.sepsize > 0) thickness = item.sepsize;
    if(thickness < 1) thickness = 1;
    const QColor color = item.hasSepBg ? item.sepbg : item.fg;

    w.startItem(row, col, colspan);
    w.startWidget("Line", uniqueName("line"));
    w.sizeProperty("minimumSize", 0, thickness);
    w.sizeProperty("maximumSize", 16777215, thickness);
    w.enumProperty("frameShadow", "QFrame::Plain");
    w.numberProperty("lineWidth", thickness);
    w.enumProperty("orientation", "Qt::Horizontal");
    w.stringProperty("styleSheet", "color: " + color.name(), true);
    w.endWidget();
    w.endItem();
}

void ParsePrcFile::emitFormReadContent(PrcUiWriter &w, const Item &item)
{
    if(!item.calc.isEmpty()) {
        // computed readback: hidden caCalc evaluates, caLineEdit displays;
        // pep expressions reference the channel as "value" -> calc input A
        QString calcExpr = item.calc;
        calcExpr.replace(QRegularExpression(QStringLiteral("\\bvalue\\b"),
                         QRegularExpression::CaseInsensitiveOption), QStringLiteral("A"));
        const QString variable = uniqueName("prc_calc");
        w.startItem();
        w.startWidget("caCalc", uniqueName("cacalc"));
        w.sizeProperty("maximumSize", 0, 0);
        w.stringProperty("variable", variable);
        w.stringProperty("calc", calcExpr);
        w.stringProperty("channel", item.channel);
        w.endWidget();
        w.endItem();
        w.startItem();
        writeLineEditWidget(w, item, variable, item.fmt[0], metrics.lineEditMinWidth);
        w.endItem();
    } else {
        w.startItem();
        writeLineEditWidget(w, item, item.channel, item.fmt[0], metrics.lineEditMinWidth);
        w.endItem();
    }
    if(!item.cegu.isEmpty()) {
        w.startItem();
        writeLabelWidget(w, item, item.cegu, true);
        w.endItem();
    }
}

void ParsePrcFile::emitSetRdbkContent(PrcUiWriter &w, const Item &item)
{
    // pep semantics: two formats = set + readback, one format = readback only
    PrcFormat setFormat, rdbkFormat;
    if(item.nfmt == 2) { setFormat = item.fmt[0]; rdbkFormat = item.fmt[1]; }
    else if(item.nfmt == 1) { rdbkFormat = item.fmt[0]; }

    const QString base = item.channel.split(QLatin1Char(':'), PRC_SKIP_EMPTY).at(0);

    w.startItem();
    writeSpinboxWidget(w, item, item.channel, setFormat);
    w.endItem();

    w.startItem();
    writeCompareStack(w, base + ":I-COMP");
    w.endItem();

    w.startItem();
    writeLineEditWidget(w, item, base + ":I-READ", rdbkFormat, metrics.lineEditMinWidth);
    w.endItem();

    // note: the invisible :HYS-CYDIR widget of the old parser is dropped on purpose

    w.startItem();
    w.startWidget("caChoice", uniqueName("cachoice"));
    w.stringProperty("channel", base + ":ONOFF");
    w.sizeProperty("minimumSize", metrics.menuMinWidth, metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    w.enumProperty("stackingMode", "caChoice::Column");
    w.enumProperty("colorMode", "caChoice::Alarm");
    w.colorProperty("bordercolor", QColor(0, 0, 0), 255);
    w.endWidget();
    w.endItem();

    w.startItem();
    writeLineEditWidget(w, item, base + ":PS-MODE", PrcFormat(), metrics.lineEditMinWidth);
    w.endItem();
}

void ParsePrcFile::emitWheelSwitchContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    writeSpinboxWidget(w, item, item.channel, item.fmt[0]);
    w.endItem();
}

void ParsePrcFile::emitLedContent(PrcUiWriter &w, const Item &item)
{
    // parse the -ledstate list "value color value color ..."
    QStringList stateTokens;
    if(!item.ledstate.isEmpty())
        stateTokens = item.ledstate.split(QLatin1Char(' '), PRC_SKIP_EMPTY);
    const int nbStates = stateTokens.count() / 2;

    const int frame = metrics.ledFrameSize;
    const int led = metrics.ledSize;
    const int off = (frame - led) / 2;

    w.startItem();
    w.startWidget("caFrame", uniqueName("caframe"));
    w.sizeProperty("minimumSize", frame, frame);
    w.sizeProperty("maximumSize", frame, frame);

    if(nbStates > 2) {
        // one led per state, stacked; the visibility calc raises the matching one
        for(int s = 0; s < nbStates; s++) {
            const QString value = stateTokens.at(2 * s);
            const QColor color(stateTokens.at(2 * s + 1));
            w.startWidget("caFrame", uniqueName("caledframe"));
            w.rectProperty("geometry", 0, 0, frame, frame);
            w.enumProperty("visibility", "caFrame::Calc");
            w.stringProperty("visibilityCalc", QString("A=%1").arg(value));
            w.stringProperty("channel", item.channel);
            w.startWidget("caLed", uniqueName("caled"));
            w.rectProperty("geometry", off, off, led, led);
            w.sizeProperty("minimumSize", led, led);
            w.sizeProperty("maximumSize", led, led);
            w.stringProperty("channel", item.channel);
            w.enumProperty("colorMode", "caLed::Static");
            w.colorProperty("trueColor", color, 255);
            w.colorProperty("falseColor", color, 255);
            w.endWidget();
            w.endWidget();
        }
    } else {
        QColor falseColor(160, 160, 164);
        QColor trueColor(0, 205, 0);
        QString falseValue, trueValue;
        if(nbStates >= 1) { falseValue = stateTokens.at(0); falseColor = QColor(stateTokens.at(1)); }
        if(nbStates == 2) { trueValue = stateTokens.at(2); trueColor = QColor(stateTokens.at(3)); }
        if(nbStates == 1) trueColor = falseColor;

        w.startWidget("caLed", uniqueName("caled"));
        w.rectProperty("geometry", off, off, led, led);
        w.sizeProperty("minimumSize", led, led);
        w.sizeProperty("maximumSize", led, led);
        w.stringProperty("channel", item.channel);
        w.enumProperty("colorMode", "caLed::Static");
        if(!trueValue.isEmpty()) w.stringProperty("trueValue", trueValue);
        if(!falseValue.isEmpty()) w.stringProperty("falseValue", falseValue);
        w.colorProperty("trueColor", trueColor, 255);
        w.colorProperty("falseColor", falseColor, 255);
        w.endWidget();

        if(item.ledstate.isEmpty()) {
            // alarm ring like the old parser
            w.startWidget("caGraphics", uniqueName("cagraphics"));
            w.rectProperty("geometry", 0, 0, frame, frame);
            w.sizeProperty("minimumSize", frame, frame);
            w.sizeProperty("maximumSize", frame, frame);
            w.stringProperty("channel", item.channel);
            w.enumProperty("colorMode", "caGraphics::Alarm");
            w.enumProperty("form", "caGraphics::Circle");
            w.enumProperty("fillstyle", "caGraphics::Filled");
            w.endWidget();
            w.zOrder("cagraphics");
            w.zOrder("caled");
        }
    }
    w.endWidget();   // outer frame
    w.endItem();
}

void ParsePrcFile::emitBinaryContent(PrcUiWriter &w, const Item &item)
{
    if(!item.compact) {
        w.startItem();
        writeLineEditWidget(w, item, item.channel, item.fmt[0], metrics.lineEditMinWidth);
        w.endItem();
    }
    w.startItem();
    w.startWidget("caToggleButton", uniqueName("catogglebutton"));
    w.stringProperty("channel", item.channel);
    w.sizeProperty("minimumSize", metrics.toggleMinWidth, metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    w.stringProperty("text", "toggle");
    w.enumProperty("colorMode", "caToggleButton::Alarm");
    w.endWidget();
    w.endItem();
}

void ParsePrcFile::emitTextContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    writeLineEditWidget(w, item, item.channel, item.fmt[0], metrics.lineEditMinWidth);
    w.endItem();

    w.startItem();
    writeLabelWidget(w, item, "new value", true);
    w.endItem();

    w.startItem();
    w.startWidget("caTextEntry", uniqueName("catextentry"));
    w.sizeProperty("minimumSize", metrics.lineEditMinWidth, metrics.rowHeight);
    w.setProperty("alignment", "Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter");
    w.stringProperty("channel", item.channel);
    w.enumProperty("colorMode", "caLineEdit::Static");
    w.colorProperty("background", QColor(0, 0, 0), 255);
    w.colorProperty("foreground", QColor(255, 255, 255), 255);
    w.fontProperty(prcFontFamily, metrics.fontSize);
    w.endWidget();
    w.endItem();
}

void ParsePrcFile::emitEntryContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    w.startWidget("caTextEntry", uniqueName("catextentry"));
    int minWidth = metrics.lineEditMinWidth;
    if(item.width > 0) {
        QFont font(prcFontFamily, metrics.fontSize);
        QFontMetrics fm(font);
        minWidth = item.width * fm.boundingRect(QLatin1Char('0')).width() + 8;
    }
    w.sizeProperty("minimumSize", minWidth, metrics.rowHeight);
    w.setProperty("alignment", "Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter");
    w.stringProperty("channel", item.channel);
    w.enumProperty("colorMode", "caLineEdit::Static");
    w.colorProperty("background", QColor(0, 0, 0), 255);
    w.colorProperty("foreground", QColor(255, 255, 255), 255);
    w.fontProperty(prcFontFamily, metrics.fontSize);
    w.endWidget();
    w.endItem();
}

void ParsePrcFile::emitMenuContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    w.startWidget("caMenu", uniqueName("camenu"));
    w.stringProperty("channel", item.channel);
    w.sizeProperty("minimumSize", metrics.menuMinWidth, metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    w.endWidget();
    w.endItem();
}

void ParsePrcFile::emitChoiceContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    w.startWidget("caChoice", uniqueName("cachoice"));
    w.stringProperty("channel", item.channel);
    w.sizeProperty("minimumSize", metrics.menuMinWidth, metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    w.enumProperty("stackingMode", "caChoice::Column");
    w.enumProperty("colorMode", "caChoice::Alarm");
    w.colorProperty("bordercolor", QColor(0, 0, 0), 255);
    w.endWidget();
    w.endItem();
}

void ParsePrcFile::emitSliderContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    w.startWidget("caSlider", uniqueName("caslider"));
    w.stringProperty("channel", item.channel);
    w.enumProperty("direction", "Right");
    w.sizeProperty("minimumSize", metrics.spinboxMaxWidth, metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    if(item.hasOPR) {
        w.enumProperty("limitsMode", "caSlider::User");
        w.raw().writeStartElement("property");
        w.raw().writeAttribute("name", "minValue");
        w.raw().writeTextElement("double", QString::number(item.oprMin));
        w.raw().writeEndElement();
        w.raw().writeStartElement("property");
        w.raw().writeAttribute("name", "maxValue");
        w.raw().writeTextElement("double", QString::number(item.oprMax));
        w.raw().writeEndElement();
    }
    if(item.fmt[0].valid) {
        // pep: the bare numeric token of a slider line is the step (resolution)
        QString resolution;
        if(item.fmt[0].precision >= 0) {
            resolution = QString("%1.%2").arg(qMax(0, item.fmt[0].width)).arg(item.fmt[0].precision);
            w.enumProperty("precisionMode", "caSlider::User");
            w.numberProperty("precision", item.fmt[0].precision);
        } else if(item.fmt[0].width >= 0) {
            resolution = QString::number(item.fmt[0].width);
        }
        bool ok = false;
        const double step = resolution.toDouble(&ok);
        if(ok && step > 0) {
            w.raw().writeStartElement("property");
            w.raw().writeAttribute("name", "incrementValue");
            w.raw().writeTextElement("double", QString::number(step));
            w.raw().writeEndElement();
        }
    }
    w.endWidget();
    w.endItem();

    if(item.trough) {
        w.startItem();
        writeLineEditWidget(w, item, item.channel,
                            item.troughFormat.valid ? item.troughFormat : item.fmt[0],
                            metrics.lineEditMinWidth);
        w.endItem();
    }
}

void ParsePrcFile::emitBarContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    w.startWidget("caThermo", uniqueName("cathermo"));
    w.stringProperty("channel", item.channel);
    w.enumProperty("direction", "Right");
    w.enumProperty("look", "Limits");
    w.sizeProperty("minimumSize", metrics.spinboxMaxWidth, metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    if(item.hasOPR) {
        w.enumProperty("limitsMode", "caThermo::User");
        w.raw().writeStartElement("property");
        w.raw().writeAttribute("name", "minValue");
        w.raw().writeTextElement("double", QString::number(item.oprMin));
        w.raw().writeEndElement();
        w.raw().writeStartElement("property");
        w.raw().writeAttribute("name", "maxValue");
        w.raw().writeTextElement("double", QString::number(item.oprMax));
        w.raw().writeEndElement();
    }
    w.endWidget();
    w.endItem();

    // numeric readback next to the bar (format from -printvar or the format token)
    PrcFormat valueFormat = item.fmt[0];
    if(!item.printvar.isEmpty()) {
        const PrcFormat f = PrcTokenizer::parseFormat(item.printvar);
        if(f.valid) valueFormat = f;
    }
    w.startItem();
    writeLineEditWidget(w, item, item.channel, valueFormat, metrics.lineEditMinWidth);
    w.endItem();

    if(!item.cegu.isEmpty()) {
        w.startItem();
        writeLabelWidget(w, item, item.cegu, true);
        w.endItem();
    }
}

void ParsePrcFile::emitCompareContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    writeCompareStack(w, item.channel);
    w.endItem();
}

void ParsePrcFile::emitMessageButtonContent(PrcUiWriter &w, const Item &item)
{
    w.startItem();
    w.startWidget("caMessageButton", uniqueName("camessagebutton"));
    w.stringProperty("channel", item.channel);
    const QString label = item.label.isEmpty() ? QString("?") : item.label;
    w.stringProperty("label", label);
    w.stringProperty("pressMessage", item.extra.join(" "));
    w.sizeProperty("minimumSize", 10 * (label.size() + 1), metrics.buttonHeight);
    w.sizeProperty("maximumSize", 16777215, metrics.buttonHeight);
    w.endWidget();
    w.endItem();
}
