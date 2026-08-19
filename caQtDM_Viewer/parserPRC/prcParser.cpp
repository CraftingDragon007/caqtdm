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

#include <cstdio>
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QHashIterator>
#include <QtUiTools/QUiLoader>

#include "parseprcfile.h"    // caQtDM_Parsers/prcParserSrc

// caQtDM-wide idiom, locally defined (standalone CLI)
#define qasc(x) x.toLatin1().constData()

// exit codes: 0 ok, 1 usage/input, 2 empty output, 3 not writable, 4 verify failed
enum { RcOk = 0, RcUsage = 1, RcEmpty = 2, RcWrite = 3, RcVerify = 4 };

static void usage()
{
    fprintf(stderr, "usage: prc2ui [--large] [--verify] <input.prc> [output.ui]\n");
    fprintf(stderr, "  converts a PSI pep resource file (.prc) to a Qt designer .ui file\n");
    fprintf(stderr, "  --large        use the historic (large) layout metrics\n");
    fprintf(stderr, "                 (same as CAQTDM_PRC_LARGE_LAYOUT=1)\n");
    fprintf(stderr, "  --verify       try to load the generated file with QUiLoader\n");
    fprintf(stderr, "  default output: <input basename>.ui in the current directory\n");
}

static int writeOut(const QByteArray &data, const QString &inputFile, const QString &outputFile)
{
    if(data.isEmpty()) {
        fprintf(stderr, "prc2ui error: parsing %s produced no output\n", qasc(inputFile));
        return RcEmpty;
    }
    QFile out(outputFile);
    if(!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "prc2ui error: cannot write %s\n", qasc(outputFile));
        return RcWrite;
    }
    out.write(data);
    out.close();
    printf("prc2ui: %s -> %s (%lld bytes)\n", qasc(inputFile), qasc(outputFile), (long long) data.size());
    return RcOk;
}

static int verifyFile(const QString &outputFile)
{
    QFile f(outputFile);
    if(!f.open(QIODevice::ReadOnly)) return RcVerify;
    QUiLoader loader;
    QWidget *widget = loader.load(&f, Q_NULLPTR);
    f.close();
    if(widget == Q_NULLPTR) {
        fprintf(stderr, "prc2ui error: QUiLoader cannot load %s (%s)\n",
                qasc(outputFile), qasc(loader.errorString()));
        return RcVerify;
    }
    // no delete: manually deleting the partially built tree (ca classes
    // unknown without designer plugins) crashes QApplication teardown;
    // the top-level widget is cleaned up by QApplication itself
    return RcOk;
}

static void printStatistics(const ParsePrcFile &parser)
{
    QHashIterator<QString, int> it(parser.unsupportedStatistics());
    while(it.hasNext()) {
        it.next();
        fprintf(stderr, "prc2ui unsupported: %s (%d x)\n", qasc(it.key()), it.value());
    }
}

int main(int argc, char *argv[])
{
    // QGuiApplication is not enough: the parser uses QFontMetrics and
    // --verify loads widgets; run headless (no display) unless overridden
    if(qgetenv("QT_QPA_PLATFORM").isEmpty()) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    bool large = false, verify = false;
    QString inputFile, outputFile;

    for(int i = 1; i < argc; i++) {
        const QString arg = QString(argv[i]);
        if(arg == "-h" || arg == "--help") { usage(); return RcOk; }
        else if(arg == "--large") { large = true; }
        else if(arg == "--verify") { verify = true; }
        else if(arg.startsWith("--")) {
            fprintf(stderr, "prc2ui error: unknown option %s\n", qasc(arg));
            usage();
            return RcUsage;
        }
        else if(inputFile.isEmpty()) inputFile = arg;
        else if(outputFile.isEmpty()) outputFile = arg;
        else { usage(); return RcUsage; }
    }

    if(inputFile.isEmpty()) { usage(); return RcUsage; }
    if(!QFile::exists(inputFile)) {
        fprintf(stderr, "prc2ui error: input file %s does not exist\n", qasc(inputFile));
        return RcUsage;
    }

    const QString outName = outputFile.isEmpty()
            ? QFileInfo(inputFile).completeBaseName() + ".ui"
            : outputFile;

    const PrcLayoutMode mode = large ? PrcLayoutLarge : PrcLayoutFromEnv;
    ParsePrcFile parser(inputFile, false, mode);
    printStatistics(parser);
    int rc = writeOut(parser.uiData(), inputFile, outName);
    if(rc == RcOk && verify) rc = verifyFile(outName);

    return rc;
}
