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

#include "prctokenizer.h"
#include <QRegularExpression>

QString PrcFormat::toPrintf() const
{
    if(!valid) return QString();
    QString s = "%";
    if(width >= 0) s += QString::number(width);
    if(precision >= 0) s += "." + QString::number(precision);
    s += conv;
    return s;
}

QList<PrcToken> PrcTokenizer::tokenize(const QString &line)
{
    QList<PrcToken> tokens;
    QString current;
    bool quoted = false;
    bool inSingle = false, inDouble = false;
    int braceDepth = 0;
    bool haveToken = false;

    for(int i = 0; i < line.size(); i++) {
        const QChar c = line.at(i);

        if(inSingle) {
            if(c == QLatin1Char('\'')) inSingle = false;
            else current += c;
            continue;
        }
        if(inDouble) {
            if(c == QLatin1Char('"')) inDouble = false;
            else current += c;
            continue;
        }
        if(braceDepth > 0) {
            if(c == QLatin1Char('{')) { braceDepth++; current += c; }
            else if(c == QLatin1Char('}')) {
                braceDepth--;
                if(braceDepth > 0) current += c;
            }
            else current += c;
            continue;
        }

        if(c == QLatin1Char('\'')) { inSingle = true; quoted = true; haveToken = true; }
        else if(c == QLatin1Char('"')) { inDouble = true; quoted = true; haveToken = true; }
        else if(c == QLatin1Char('{')) { braceDepth = 1; quoted = true; haveToken = true; }
        else if(c == QLatin1Char(' ') || c == QLatin1Char('\t')) {
            if(haveToken) {
                PrcToken t; t.text = current; t.quoted = quoted;
                tokens.append(t);
                current.clear(); quoted = false; haveToken = false;
            }
        } else {
            current += c;
            haveToken = true;
        }
    }
    if(haveToken) {
        PrcToken t; t.text = current; t.quoted = quoted;
        tokens.append(t);
    }
    return tokens;
}

PrcFormat PrcTokenizer::parseFormat(const QString &token)
{
    PrcFormat f;
    static const QRegularExpression rx(
        QStringLiteral("^%?(\\d+)?(?:\\.(\\d+))?([defgsxo])?$"));
    const QRegularExpressionMatch m = rx.match(token);
    if(!m.hasMatch()) return f;
    const QString w = m.captured(1);
    const QString p = m.captured(2);
    const QString c = m.captured(3);
    // an empty token or a lone "%" is not a format
    if(w.isEmpty() && p.isEmpty() && c.isEmpty()) return f;
    f.valid = true;
    if(!w.isEmpty()) f.width = w.toInt();
    if(!p.isEmpty()) f.precision = p.toInt();
    if(!c.isEmpty()) f.conv = c.at(0);
    return f;
}
