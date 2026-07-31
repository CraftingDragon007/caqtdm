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
 */

#ifndef TST_ENVFILTER_H
#define TST_ENVFILTER_H

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>

class TestEnvFilter : public QObject
{
    Q_OBJECT
public:
    TestEnvFilter() = default;

private:
    QString m_stdPath;
    QString m_configName;
    QMap<QString, QByteArray> m_savedEnv;

    void rememberEnv(const QString &name);
    bool applyConfig(const QString &content);

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void appliesRealWorldConfiguration();

    void blocksDangerousNames();
    void blocksDangerousNames_data();

    void ignoresMalformedNames();
    void ignoresMalformedNames_data();

    void ignoresOversizedValue();

    void acceptsMaxLengthName();
    void acceptsMaxLengthValue();
    void ignoresLineWithoutSpace();
};

#endif // TST_ENVFILTER_H
