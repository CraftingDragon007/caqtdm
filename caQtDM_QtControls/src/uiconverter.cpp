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

#include "uiconverter.h"
#include "parsepepfile.h"    // historic .prc parser, untouched
#include "parseprcfile.h"    // new .prc parser (caQtDM_Parsers/prcParserSrc)
#ifdef ADL_EDL_FILES
#include "parseotherfile.h"  // .adl / .edl converters
#endif
#include <QFileInfo>

Q_LOGGING_CATEGORY(uiConverterLog, "caqtdm.widgets.uiconverter")

// adapters by composition - the wrapped classes stay untouched

class PepConverterAdapter : public UiConverterInterface
{
public:
    PepConverterAdapter(const QString &filename, bool willPrint)
        : pep(filename, willPrint) {}
    QWidget *load(QWidget *parent) { return pep.load(parent); }
private:
    ParsePepFile pep;
};

class PrcConverterAdapter : public UiConverterInterface
{
public:
    PrcConverterAdapter(const QString &filename, bool willPrint)
        : prc(filename, willPrint) {}
    QWidget *load(QWidget *parent) { return prc.load(parent); }
    QString title() const { return prc.title(); }
    bool ok() const { return prc.ok(); }
private:
    ParsePrcFile prc;
};

#ifdef ADL_EDL_FILES
class OtherFileAdapter : public UiConverterInterface
{
public:
    OtherFileAdapter(const QString &filename) : okFlag(false)
    {
        other = new ParseOtherFile(filename, okFlag, error);
    }
    ~OtherFileAdapter() { delete other; }
    QWidget *load(QWidget *parent) { return okFlag ? other->load(parent) : (QWidget *) Q_NULLPTR; }
    bool ok() const { return okFlag; }
    QString errorString() const { return error; }
private:
    ParseOtherFile *other;
    bool okFlag;
    QString error;
};
#endif

QString UiConverterFactory::selectionFor(const QString &extension)
{
    const QString ext = extension.toLower();
    QString value = QString(qgetenv(QString("CAQTDM_CONVERTER_%1")
                            .arg(ext.toUpper()).toLatin1().constData())).trimmed().toLower();
    // compatibility switch for the prc development phase
    if(ext == "prc" && value.isEmpty()) {
        if(qgetenv("CAQTDM_PRC_CONVERTER").trimmed() == "1") value = "new";
    }
    return value;
}

bool UiConverterFactory::handles(const QString &fileName)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if(suffix == "prc") return true;
#ifdef ADL_EDL_FILES
    if(suffix == "adl" || suffix == "edl") return true;
#endif
    return false;
}

UiConverterInterface *UiConverterFactory::create(const QString &fileName, bool willPrint)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();

    if(suffix == "prc") {
        const bool useNew = (selectionFor(suffix) == "new");
        static bool logged = false;
        if(!logged) {
            logged = true;
            qCInfo(uiConverterLog) << "prc converter implementation:"
                                   << (useNew ? "new (ParsePrcFile)" : "old (ParsePepFile)");
        }
        if(useNew) return new PrcConverterAdapter(fileName, willPrint);
        return new PepConverterAdapter(fileName, willPrint);
    }
#ifdef ADL_EDL_FILES
    if(suffix == "adl" || suffix == "edl") return new OtherFileAdapter(fileName);
#endif

    qCWarning(uiConverterLog) << "no converter registered for" << fileName;
    return (UiConverterInterface *) Q_NULLPTR;
}
