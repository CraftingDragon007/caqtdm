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
 *  Copyright (c) 2010 - 2025
 *
 *  Author:
 *    Helge Brands
 *  Contact details:
 *    helge.brands@psi.ch
 */

#ifndef caWaveTableModel_H
#define caWaveTableModel_H

#include <QAbstractTableModel>

class caWaveTableModel : public QAbstractTableModel
{

public:
    explicit caWaveTableModel(int rows, int columns, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    QString getHorizontalString() const;
    void setHorizontalString(const QString &newHorizontalString);

    QString getVerticalString() const;
    void setVerticalString(const QString &newVerticalString);

    int getHorizontalOffset() const;
    void setHorizontalOffset(int newHorizontalOffset);

    int getVerticalOffset() const;
    void setVerticalOffset(int newVerticalOffset);

private:
    QList<QStringList> rowList;
    QString horizontalString;
    QString verticalString;
    int horizontalOffset;
    int verticalOffset;
};

#endif //caWaveTableModel_H
