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

#include "caWaveTableModel.h"


caWaveTableModel::caWaveTableModel(int rows, int columns,QObject *parent)
    : QAbstractTableModel(parent)
{
    QStringList newList;

    for (int column = 0; column < qMax(1, columns); ++column) {
        newList.append(QString());
    }

    for (int row = 0; row < qMax(1, rows); ++row) {
        rowList.append(newList);
    }
    verticalOffset=1;
    horizontalOffset=1;
    horizontalString="";
    verticalString="";

}


//-------------------------------------------------------
int caWaveTableModel::rowCount(const QModelIndex & /*parent*/) const
{
    return rowList.size();
}

//-------------------------------------------------------
int caWaveTableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return rowList[0].size();
}

//-------------------------------------------------------
QVariant caWaveTableModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        return QString("Row%1, Column%2")
            .arg(index.row() + 1)
            .arg(index.column() +1);
    }
    return QVariant();
}


QVariant caWaveTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{



    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        QString headerstring=horizontalString;
        if (headerstring.contains(";")){
            QList<QString> data=headerstring.split(";");
            if (section < data.length()){
                headerstring=data.at(section);
            }else{
                headerstring="not defined";
            }
        }
        if (headerstring.contains("%1")){
            return QString(headerstring).arg(section+horizontalOffset);
        } else if (headerstring.isEmpty()){
            return QString("%1").arg(section+horizontalOffset);
        } else{
            return QString(headerstring);
        }
     }

    if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
        QString  headerstring=verticalString;
        if (headerstring.contains(";")){
            QList<QString> data=headerstring.split(";");
            if (section < data.length()){
                headerstring=data.at(section);
            }else{
                headerstring="not defined";
            }
        }


         if (headerstring.contains("%1")){
             return QString(headerstring).arg(section+verticalOffset);
         } else if (headerstring.isEmpty()){
             return QString("%1").arg(section+verticalOffset);
         } else{
             return QString(headerstring);
         }
    }





    return QVariant();
}

QString caWaveTableModel::getHorizontalString() const
{
    return horizontalString;
}

void caWaveTableModel::setHorizontalString(const QString &newHorizontalString)
{
    horizontalString = newHorizontalString;
}

QString caWaveTableModel::getVerticalString() const
{
    return verticalString;
}

void caWaveTableModel::setVerticalString(const QString &newVerticalString)
{
    verticalString = newVerticalString;
}

int caWaveTableModel::getHorizontalOffset() const
{
    return horizontalOffset;
}

void caWaveTableModel::setHorizontalOffset(int newHorizontalOffset)
{
    horizontalOffset = newHorizontalOffset;
}

int caWaveTableModel::getVerticalOffset() const
{
    return verticalOffset;
}

void caWaveTableModel::setVerticalOffset(int newVerticalOffset)
{
    verticalOffset = newVerticalOffset;
}

#include "moc_cawavetableModel.cpp"
