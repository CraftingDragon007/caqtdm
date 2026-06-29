/*
 *  This file is part of the caQtDM Framework.
 */

#ifndef CA3DCONFIGDIALOG_H
#define CA3DCONFIGDIALOG_H

#include <QDialog>
#include <qtcontrols_global.h>

class ca3DWidget;
class QDialogButtonBox;
class QComboBox;
class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QPixmap;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QWidget;

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
    void addOverlayRow();
    void removeOverlayRow();
    void addPresetRow();
    void removePresetRow();
    void editSelectedBindingPv();
    void refreshPreview();
    void captureSnapshot();
    void finishSnapshotCapture(const QPixmap &snapshot);
    void failSnapshotCapture(const QString &error);
    void validateRawJson();
    void applyChanges();
    void markChanged();
    void accept() override;

private:
    void buildUi();
    void loadFromWidget();
    void populateTablesFromJson(const QString &json);
    void populatePresetSelector(const QString &json);
    QString currentEditorJson() const;
    QString jsonFromTables() const;
    void updateRawJsonFromTables();
    static bool validateJson(const QString &json, QStringList *errors);
    QString tableText(QTableWidget *table, int row, int column) const;
    void setTableText(QTableWidget *table, int row, int column, const QString &text);
    void setTableCombo(QTableWidget *table, int row, int column, const QStringList &items, const QString &currentText);
    QString tableComboText(QTableWidget *table, int row, int column) const;
    void setTableCheck(QTableWidget *table, int row, int column, bool checked);
    bool tableCheck(QTableWidget *table, int row, int column) const;
    void showErrors(const QStringList &errors);

    ca3DWidget *widget3D;
    ca3DWidget *previewWidget;
    QComboBox *previewPresetCombo;
    QPushButton *captureSnapshotButton;
    QString pendingSnapshotFileName;
    int pendingSnapshotPreset;
    QTabWidget *tabs;
    QTableWidget *objectsTable;
    QTableWidget *bindingsTable;
    QTableWidget *overlaysTable;
    QTableWidget *presetsTable;
    QPlainTextEdit *rawJsonEdit;
    QLabel *rawValidationLabel;
    QLabel *errorLabel;
    QDialogButtonBox *buttonBox;
    bool updatingUi;
};

#endif
