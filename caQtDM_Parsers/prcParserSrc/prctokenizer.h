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

#ifndef PRCTOKENIZER_H
#define PRCTOKENIZER_H

#include <QString>
#include <QList>
#include "prcparserdefs.h"

// one whitespace-separated token; quoted = was enclosed in '', "" or {}
struct PrcToken {
    QString text;
    bool quoted;
};

// printf-like format token: 9.3 / 9.2e / %8.3f / 4.1f / s / x / o
struct PrcFormat {
    bool valid;
    int width;      // -1 = not given
    int precision;  // -1 = not given
    QChar conv;     // f (default), d, e, g, s, x, o

    PrcFormat() : valid(false), width(-1), precision(-1), conv(QLatin1Char('f')) {}
    QString toPrintf() const;
};

class PRCPARSER_EXPORT PrcTokenizer
{
public:
    // split a line honoring '…', "…" and nested {…}; tabs count as blanks
    static QList<PrcToken> tokenize(const QString &line);
    static PrcFormat parseFormat(const QString &token);
};

#endif // PRCTOKENIZER_H
