// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "caWaveTableModel.h"
#pragma optimize( "", off )
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
        if (horizontalString.contains("%1")){
            return QString(horizontalString).arg(section+horizontalOffset);
        } else if (horizontalString.isEmpty()){
            return QString("%1").arg(section+horizontalOffset);
        } else{
            return QString(horizontalString);
        }
     }

    if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
         if (verticalString.contains("%1")){
             return QString(verticalString).arg(section+verticalOffset);
         } else if (verticalString.isEmpty()){
             return QString("%1").arg(section+verticalOffset);
         } else{
             return QString(verticalString);
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
