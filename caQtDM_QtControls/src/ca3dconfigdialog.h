/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DCONFIGDIALOG_H
#define CA3DCONFIGDIALOG_H

#include <QDialog>
#include <qtcontrols_global.h>

class ca3DWidget;
class QDialogButtonBox;
class QLabel;
class QPlainTextEdit;
class QTableWidget;
class QTabWidget;

class QTCON_EXPORT ca3DConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ca3DConfigDialog(ca3DWidget *widget, QWidget *parent = 0);

private slots:
    void addObjectRow();
    void removeObjectRow();
    void addBindingRow();
    void removeBindingRow();
    void validateRawJson();
    void applyChanges();
    void accept() override;

private:
    void buildUi();
    void loadFromWidget();
    void populateTablesFromJson(const QString &json);
    QString jsonFromTables() const;
    void updateRawJsonFromTables();
    bool validateJson(const QString &json, QStringList *errors) const;
    QString tableText(QTableWidget *table, int row, int column) const;
    void setTableText(QTableWidget *table, int row, int column, const QString &text);
    void setTableCombo(QTableWidget *table, int row, int column, const QStringList &items, const QString &currentText);
    QString tableComboText(QTableWidget *table, int row, int column) const;
    void showErrors(const QStringList &errors);

    ca3DWidget *widget3D;
    QTabWidget *tabs;
    QTableWidget *objectsTable;
    QTableWidget *bindingsTable;
    QPlainTextEdit *rawJsonEdit;
    QLabel *errorLabel;
    QDialogButtonBox *buttonBox;
};

#endif
