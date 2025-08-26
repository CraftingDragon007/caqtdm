// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef caWaveTableModel_H
#define caWaveTableModel_H

#include <QAbstractTableModel>

class caWaveTableModel : public QAbstractTableModel
{
    Q_OBJECT
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

#endif // MYMODEL_H
