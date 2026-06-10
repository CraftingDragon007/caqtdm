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
 *    Erik Schwarz
 *  Contact details:
 *    erik.schwarz@psi.ch
 */

#ifndef TST_PVDIALOG_H
#define TST_PVDIALOG_H

#include "fakeformwindow.h"
#include <pvdialog.h>

#include <QObject>
#include <QTest>

class TestPVDialog : public QObject
{
    Q_OBJECT
public:
    TestPVDialog() = default;

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void cleanup();
    void savesPlainChannelIfNothingIsSet();
    void savesPrefix();
    void savesDeadband();
    void savesMaxDisplayRate();
    void savesDecimation();
    void savesArray();
    void savesSync();
    void savesTs();
    void savesEverythingAtOnce();
    void savesNothingWithoutPV();
    void constructorParsesPlainChannel();
    void constructorParsesSomeFilters();
    void constructorParsesAllFilters();
    void roundTripSaveThenParse();

private:
    void parseChannel(const QString &channel, QString &outPv, QJsonObject &outJson);
    QString getChannelData();

    PVDialog *m_dialog;
    FakeFormWindow *m_formWindow;
};

#endif // TST_PVDIALOG_H
